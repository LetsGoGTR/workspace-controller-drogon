#include "WorkspaceService.h"

#include <archive.h>
#include <archive_entry.h>
#include <dirent.h>
#include <sys/stat.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "../utils/Config.h"

namespace fs = std::filesystem;

namespace services {

// Directory traversal - recursively add to archive
static void addDirToArchive(archive* a, const std::string& path,
                            const std::string& prefix, int depth = 0) {
    // Recursion depth limit
    if (depth > Config::MAX_RECURSION_DEPTH) {
        throw std::runtime_error("Maximum directory depth exceeded");
    }

    DIR* dir = opendir(path.c_str());
    if (!dir) {
        throw std::runtime_error("Cannot open directory");
    }

    try {
        struct dirent* ent;
        while ((ent = readdir(dir))) {
            std::string name = ent->d_name;
            if (name == "." || name == "..") {
                continue;
            }

            std::string full = path + "/" + name;
            std::string arch = prefix + "/" + name;
            struct stat st;

            // lstat: don't follow symlinks
            if (lstat(full.c_str(), &st) != 0) {
                continue;
            }

            // Skip symlinks
            if (S_ISLNK(st.st_mode)) {
                continue;
            }

            // Create entry
            archive_entry* entry = archive_entry_new();
            if (!entry) {
                throw std::runtime_error("Failed to create archive entry");
            }

            archive_entry_set_pathname(entry, arch.c_str());
            archive_entry_copy_stat(entry, &st);

            // Write header
            if (archive_write_header(a, entry) != ARCHIVE_OK) {
                archive_entry_free(entry);
                throw std::runtime_error("Failed to write archive header");
            }

            // Regular file: write data
            if (S_ISREG(st.st_mode)) {
                std::ifstream file(full, std::ios::binary);
                char buf[Config::FILE_BUFFER_SIZE];
                while (file.read(buf, sizeof(buf)) || file.gcount() > 0) {
                    ssize_t written = archive_write_data(a, buf, file.gcount());
                    if (written < 0) {
                        archive_entry_free(entry);
                        throw std::runtime_error("Failed to write archive data");
                    }
                }
            }

            archive_entry_free(entry);

            // Directory: recurse
            if (S_ISDIR(st.st_mode)) {
                addDirToArchive(a, full, arch, depth + 1);
            }
        }
        closedir(dir);
    } catch (...) {
        closedir(dir);
        throw;
    }
}

// Compress: workspace -> tgz
ServiceResult WorkspaceService::compress(const std::string& user) {
    std::string base = Config::PATH_HOME_BASE + user;
    std::string workspace = base + Config::PATH_WORKSPACE;
    std::string output = base + Config::PATH_OUTPUT;

    if (!fs::exists(workspace)) {
        return ServiceResult::createError("Workspace directory does not exist");
    }

    fs::remove(output);

    archive* a = archive_write_new();
    if (!a) {
        return ServiceResult::createError("Failed to create archive");
    }

    try {
        // gzip + pax format
        archive_write_add_filter_gzip(a);
        archive_write_set_format_pax_restricted(a);

        if (archive_write_open_filename(a, output.c_str()) != ARCHIVE_OK) {
            throw std::runtime_error("Failed to open output");
        }

        addDirToArchive(a, workspace, "workspace");

        archive_write_close(a);
        archive_write_free(a);

        // Permission 644
        if (chmod(output.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
            return ServiceResult::createSuccessWithMessage(
                "Archive created successfully, but failed to set permissions to 644");
        }

        return ServiceResult::createSuccessWithMessage("Compressed");
    } catch (const std::exception& e) {
        archive_write_close(a);
        archive_write_free(a);
        return ServiceResult::createError(e.what());
    }
}

// Extract: tgz -> workspace
ServiceResult WorkspaceService::extract(const std::string& user) {
    std::string base = Config::PATH_HOME_BASE + user;
    std::string workspace = base + Config::PATH_WORKSPACE;
    std::string input = base + Config::PATH_INPUT;

    if (!fs::exists(input)) {
        return ServiceResult::createError("Archive file does not exist");
    }

    // Zip Bomb defense: archive size limit
    auto archive_size = fs::file_size(input);
    if (archive_size > Config::MAX_ARCHIVE_SIZE) {
        return ServiceResult::createError("Archive file too large");
    }

    chmod(input.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    // Backup path generation
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                         now.time_since_epoch())
                         .count();
    std::string backup_path =
        std::string(Config::PATH_BACKUP_BASE) + "_" + user + "_" +
        std::to_string(timestamp);

    // Workspace backup
    bool backup_created = false;
    if (fs::exists(workspace)) {
        fs::rename(workspace, backup_path);
        backup_created = true;
        if (fs::exists(workspace)) {
            fs::remove_all(workspace);
        }
    }

    archive* a = archive_read_new();
    if (!a) {
        if (backup_created) {
            fs::rename(backup_path, workspace);
        }
        return ServiceResult::createError("Failed to create reader");
    }

    try {
        archive_read_support_filter_all(a);
        archive_read_support_format_all(a);

        if (archive_read_open_filename(a, input.c_str(),
                                       Config::ARCHIVE_BLOCK_SIZE) != ARCHIVE_OK) {
            throw std::runtime_error(
                "Failed to open archive: Check file permissions or file integrity");
        }

        archive_entry* entry;
        size_t total_extracted = 0;

        // Iterate entries
        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            const char* pathname = archive_entry_pathname(entry);
            if (!pathname) {
                continue;
            }
            std::string path = base + "/" + pathname;
            archive_entry_set_pathname(entry, path.c_str());

            archive* ext = archive_write_disk_new();
            if (!ext) {
                throw std::runtime_error("Failed to create disk writer");
            }

            // Path Traversal defense
            archive_write_disk_set_options(
                ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
                     ARCHIVE_EXTRACT_SECURE_NODOTDOT |
                     ARCHIVE_EXTRACT_SECURE_NOABSOLUTEPATHS);

            if (archive_write_header(ext, entry) == ARCHIVE_OK) {
                const void* buf;
                size_t size;
                int64_t offset;

                while (archive_read_data_block(a, &buf, &size, &offset) == ARCHIVE_OK) {
                    // Zip Bomb defense: extract size limit
                    total_extracted += size;
                    if (total_extracted > Config::MAX_EXTRACT_SIZE) {
                        archive_write_close(ext);
                        archive_write_free(ext);
                        throw std::runtime_error("Extracted size exceeds limit");
                    }

                    if (archive_write_data_block(ext, buf, size, offset) != ARCHIVE_OK) {
                        archive_write_close(ext);
                        archive_write_free(ext);
                        throw std::runtime_error("Failed to write data block");
                    }
                }
            }

            archive_write_close(ext);
            archive_write_free(ext);
        }

        archive_read_close(a);
        archive_read_free(a);
        a = nullptr;

        fs::remove(input);

        // Workspace validation
        if (!fs::exists(workspace) || !fs::is_directory(workspace)) {
            if (backup_created) {
                fs::rename(backup_path, workspace);
                backup_created = false;
            }
            throw std::runtime_error("Workspace folder not created");
        }

        // Delete backup
        if (backup_created) {
            fs::remove_all(backup_path);
        }

        return ServiceResult::createSuccessWithMessage("Extracted successfully");
    } catch (const std::exception& e) {
        if (a) {
            archive_read_free(a);
        }

        // Restore backup
        if (backup_created) {
            if (fs::exists(workspace)) {
                fs::remove_all(workspace);
            }
            fs::rename(backup_path, workspace);
            return ServiceResult::createError("Extraction failed. Restored from backup");
        }

        return ServiceResult::createError(e.what());
    }
}

}  // namespace services

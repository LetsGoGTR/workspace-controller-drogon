#pragma once

#include <string>

#include "ServiceResult.h"

namespace services {

class WorkspaceService {
  public:
    static ServiceResult compress(const std::string& user);
    static ServiceResult extract(const std::string& user);
};

}  // namespace services

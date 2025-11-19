#pragma once

#include <json/json.h>
#include <string>

namespace services {

struct ServiceResult {
    bool success;
    std::string errorMessage;
    Json::Value data;

    ServiceResult() : success(false), errorMessage(""), data(Json::objectValue) {}

    static ServiceResult createSuccess(const Json::Value& resultData = Json::objectValue) {
        ServiceResult result;
        result.success = true;
        result.data = resultData;
        return result;
    }

    static ServiceResult createSuccessWithMessage(const std::string& message) {
        ServiceResult result;
        result.success = true;
        result.data["message"] = message;
        return result;
    }

    static ServiceResult createError(const std::string& message) {
        ServiceResult result;
        result.success = false;
        result.errorMessage = message;
        return result;
    }
};

}  // namespace services

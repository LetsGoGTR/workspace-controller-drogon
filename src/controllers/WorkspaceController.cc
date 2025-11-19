#include "WorkspaceController.h"

#include "../services/WorkspaceService.h"
#include "../utils/PathValidator.h"

namespace api {
namespace workspace {

void Workspace::compress(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto jsonPtr = req->getJsonObject();
    Json::Value response;

    if (!jsonPtr) {
        response["success"] = false;
        response["message"] = "Invalid JSON body";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    try {
        std::string user = utils::validateUser((*jsonPtr)["user"].asString());
        auto result = services::WorkspaceService::compress(user);

        if (result.success) {
            response["success"] = true;
            response["message"] = result.data.get("message", "Compressed").asString();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            callback(resp);
        } else {
            response["success"] = false;
            response["message"] = result.errorMessage;
            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        }
    } catch (const std::invalid_argument& e) {
        response["success"] = false;
        response["message"] = e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void Workspace::extract(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto jsonPtr = req->getJsonObject();
    Json::Value response;

    if (!jsonPtr) {
        response["success"] = false;
        response["message"] = "Invalid JSON body";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    try {
        std::string user = utils::validateUser((*jsonPtr)["user"].asString());
        auto result = services::WorkspaceService::extract(user);

        if (result.success) {
            response["success"] = true;
            response["message"] = result.data.get("message", "Extracted successfully").asString();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            callback(resp);
        } else {
            response["success"] = false;
            response["message"] = result.errorMessage;
            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        }
    } catch (const std::invalid_argument& e) {
        response["success"] = false;
        response["message"] = e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
    } catch (const std::exception& e) {
        response["success"] = false;
        response["message"] = e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

}  // namespace workspace
}  // namespace api

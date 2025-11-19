#include "RobotController.h"

namespace api {
namespace robot {

void Robot::running(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    Json::Value response;
    response["success"] = true;
    response["data"] = false;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

}  // namespace robot
}  // namespace api

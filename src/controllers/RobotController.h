#pragma once

#include <drogon/HttpController.h>

namespace api {
namespace robot {

class Robot : public drogon::HttpController<Robot> {
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(Robot::running, "/running", drogon::Get);
    METHOD_LIST_END

    void running(const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

}  // namespace robot
}  // namespace api

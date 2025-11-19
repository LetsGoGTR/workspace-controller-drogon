#pragma once

#include <drogon/HttpController.h>

namespace api {
namespace workspace {

class Workspace : public drogon::HttpController<Workspace> {
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(Workspace::compress, "/compress", drogon::Post);
    METHOD_ADD(Workspace::extract, "/extract", drogon::Post);
    METHOD_LIST_END

    void compress(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void extract(const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

}  // namespace workspace
}  // namespace api

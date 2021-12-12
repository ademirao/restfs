#ifndef HTTP_H
#define HTTP_H

#include <curl/curl.h>
#include <functional>
#include <json/json.h>
#include <memory>
#include <sstream>
#include <string>

namespace http {
struct Response final {
  Response() : http_code(-1), data() {}
  int http_code;
  std::stringstream data;
};

typedef std::function<void(const Response &)> Callback;

class Request final {
public:
  std::unique_ptr<Response> fetch(const std::string &url);

private:
  const std::string method_ = "GET";

  const struct curl_slist *headers_;
};
} // namespace http

#endif
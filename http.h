#ifndef HTTP_H
#define HTTP_H

#include <curl/curl.h>
#include <functional>
#include <json/json.h>
#include <memory>
#include <sstream>
#include <string>

#include "rest.h"

namespace http {

struct Response final {
  Response() : http_code(-1), data() {}
  int http_code;
  std::stringstream data;
};

typedef std::function<void(const Response &)> Callback;

class Request final {
public:
  Request(const rest::constants::OPERATIONS operation = rest::constants::GET)
      : operation_(operation) {}
  Response fetch(const std::string &url);

private:
  const rest::constants::OPERATIONS operation_;
};
} // namespace http

#endif
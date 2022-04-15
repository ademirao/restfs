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

using Callback = std::function<void(const Response &)>;

class Headers {
public:
  Headers() : headers_(nullptr) {}
  ~Headers() { curl_slist_free_all(headers_); }
  Headers &AppendHeaderLine(const char *header_line) {
    headers_ = curl_slist_append(headers_, header_line);
    return *this;
  }

private:
  struct curl_slist *headers_;
};

class Request final {
public:
  Request(const rest::constants::OPERATIONS operation = rest::constants::GET,
          const Headers *headers = nullptr)
      : operation_(operation), curl_(curl_easy_init()), headers_(headers) {
    CHECK(curl_ != NULL);
  }
  Response fetch(const std::string &url) const;

private:
  struct CurlCleanup {
    void operator()(CURL *curl) { curl_easy_cleanup(curl); }
  };
  const rest::constants::OPERATIONS operation_;
  const std::unique_ptr<CURL, CurlCleanup> curl_;
  const Headers *headers_;
};
} // namespace http

#endif
#include "http.h"
#include "logger.h"
#include <curl/curl.h>
#include <sstream>
#include <algorithm>

namespace http {
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb,
                                  Response *resp) {
  const size_t realsize = size * nmemb;
  resp->data << std::string((const char *)contents, realsize);
  LOG(INFO) << "Response: " << resp->data.str();
  return realsize;
}

Response Request::fetch(const std::string &url) const {
  Response response;
  CURL *curl = curl_.get();
  std::string operation_str = rest::constants::OPERATION_NAMES[operation_];

  std::transform(operation_str.begin(), operation_str.end(),
                 operation_str.begin(), ::toupper);
  LOG(INFO) << "OPERATION: " << operation_str;
  CHECK(curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, operation_str.c_str()) ==
        CURLE_OK);
  CHECK(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_) == CURLE_OK);
  CHECK(curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) == CURLE_OK);
  CHECK(curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5) == CURLE_OK);
  CHECK(curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L) == CURLE_OK);
  CHECK(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback) ==
        CURLE_OK);
  // Below we set the parameter to be passed to WriteMemoryCallback
  CHECK(curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response) == CURLE_OK);
  LOG(INFO) << "Fetching: " << url;
  CHECK(curl_easy_perform(curl) == CURLE_OK);
  CHECK(curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
                          &(response.http_code)) == CURLE_OK);
  LOG(INFO) << "Fetched (Code: " << response.http_code << ")";
  return response;
}

} // namespace http
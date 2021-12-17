#include "http.h"
#include "logger.h"
#include <curl/curl.h>
#include <sstream>

namespace http {
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb,
                                  Response *resp) {
  const size_t realsize = size * nmemb;
  resp->data << std::string((const char *)contents, realsize);
  return realsize;
}

std::unique_ptr<Response> Request::fetch(const std::string &url) {
  auto response = std::make_unique<Response>();
  CURL *curl = curl_easy_init();
  CHECK(curl != NULL);
  struct curl_slist *headers = curl_slist_append(NULL, getenv("API_TOKEN"));
  CHECK(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers) == CURLE_OK);
  CHECK(curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) == CURLE_OK);
  CHECK(curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5) == CURLE_OK);
  CHECK(curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L) == CURLE_OK);
  CHECK(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback) ==
        CURLE_OK);
  // Below we set the parameter to be passed to WriteMemoryCallback
  CHECK(curl_easy_setopt(curl, CURLOPT_WRITEDATA, response.get()) == CURLE_OK);
  LOG(INFO) << "Fetching: " << url;
  CHECK(curl_easy_perform(curl) == CURLE_OK);
  CHECK(curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
                          &(response->http_code)) == CURLE_OK);
  LOG(INFO) << "Fetched (Code: " << response->http_code << ")";
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  return std::move(response);
}

} // namespace http
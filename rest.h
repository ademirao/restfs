
#ifndef REST_H
#define REST_H

#include "logger.h"

#include <json/json.h>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace rest::constants {
enum OPERATIONS {
  INVALID = -1,
  GET = 0,
  PUT = 1,
  POST = 2,
  DELETE = 3,
  OPTIONS = 4,
  HEAD = 5,
  PATCH = 6,
  TRACE = 7,
};

static const char *OPERATION_NAMES[] = {"get",     "put",  "post",  "delete",
                                 "options", "head", "patch", "trace"};

const std::unordered_map<std::string, OPERATIONS> &operations_map();
std::vector<OPERATIONS> operations(const Json::Value &value);

} // namespace rest::constants

#endif
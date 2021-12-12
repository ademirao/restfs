#include "rest.h"

namespace rest::constants {

static std::once_flag once_to_map_flag;
const std::unordered_map<std::string, OPERATIONS> &operations_map() {
  static std::unordered_map<std::string, OPERATIONS> NAME_TO_OPERATION_MAP;
  std::call_once(once_to_map_flag, []() {
    for (int idx = 0; idx < sizeof(OPERATION_NAMES) / sizeof(const char *);
         ++idx) {
      NAME_TO_OPERATION_MAP[OPERATION_NAMES[idx]] = (OPERATIONS)idx;
    }
  });
  return NAME_TO_OPERATION_MAP;
}

std::vector<OPERATIONS> operations(const Json::Value &value) {
  std::vector<OPERATIONS> ops;
  for (const char *name : OPERATION_NAMES) {
    if (value.isMember(name)) {
      const auto &op_map = operations_map();
      const auto it = op_map.find(name);
      CHECK(it != op_map.end());
      ops.push_back(it->second);
    }
  }
  return ops;
}
} // namespace rest::constants
#ifndef PATHS_H
#define PATHS_H

#include "logger.h"
#include "rest.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <sys/stat.h>
#include <unordered_map>
#include <unordered_set>

namespace path {
using Path = std::filesystem::path;
template <typename ValueType>
using RefValueMap = std::unordered_map<std::string, const ValueType>;
template <typename ValueType> using PathJsonPair = std::pair<Path, RefValueMap>;

// ParamName and ParamReference share same implementation, but they mean
// different concepts. The "reference" to a param happens to be the param name,
// but it could be implemented as the position. The typedefs below are supposed
// to capture that difference.
using ParamReference = std::string;
using ParamName = std::string;
using RefSet = std::unordered_set<path::ParamReference>;

const RefSet ReferencesSetFromPath(const path::Path &path);

class Info final {
public:
  Info(const Info &info) : stat(info.stat) {}
  Info(const struct stat &s, const Json::Value *j)
      : stat(s), json(std::move(j)) {}

  const struct stat stat;
  const Json::Value *json;
};

std::unique_ptr<const path::Info> FileInfo(const std::string &key,
                                           const Json::Value *json);
std::unique_ptr<const path::Info> DirInfo(const std::string &key,
                                          const Json::Value *json);

namespace utils {
const std::vector<path::Path> all_prefixes(const path::Path &path);

typedef const std::function<const ParamReference(const ParamReference &ref,
                                                 const std::string &value)>
    Binder;

static const ParamReference FailOnBinding(const ParamReference &ref,
                                          const std::string &value) {
  LOG(FATAL) << "Binding not allowed: " << ref << " <- " << value;
  return {}; // unreachable
};

const path::Path CanonicalizeRefs(const path::Path &path);
const RefSet RefSetFromPath(const path::Path &path);
const path::Path BindRefs(const path::Path &path, Binder binder);
} // namespace utils

} // namespace path

std::ostream &operator<<(std::ostream &os, const path::Info &s);
std::ostream &operator<<(std::ostream &os, const struct stat &s);

#endif
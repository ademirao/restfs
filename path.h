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

// ParamName and ParamReference share same implementation, but they mean
// different concepts. The "reference" to a param happens to be the param name,
// but it could be implemented as the position. The typedefs below are supposed
// to capture that difference.
using Reference = std::string;
using Ref = Reference;
using ParamName = std::string;
using ReferenceSet = std::unordered_set<path::Ref>;
using RefSet = ReferenceSet;

} // namespace path

namespace std {
template <> struct hash<path::Path> {
  std::size_t operator()(const path::Path &path) const {
    return hash_value(path);
  }
};
} // namespace std

namespace path {
class Node final {
public:
  Node(const Node &node)
      : path_(node.path_), stat_(node.stat_), children_(node.children_),
        metadata_(node.metadata_) {}
  Node(const Path& path, const struct stat &stat, const void *metadata)
      : path_(path), stat_(stat), metadata_(metadata) {}

  const Path &path() const { return path_; }

  const struct stat &stat() const { return stat_; }

  template <typename Metadata> const Metadata *metadata() const {
    return metadata_;
  }

  std::vector<const path::Node *> *mutable_children() { return &children_; }

  const std::vector<const path::Node *> &children() const { return children_; }

private:
  const Path path_;
  const struct stat stat_;
  const void *metadata_;
  std::vector<const path::Node *> children_;
};

path::Node FileNode(const path::Path &key, const Json::Value *json);
path::Node DirNode(const path::Path &key, const Json::Value *json);

namespace utils {
const std::vector<path::Path> all_prefixes(const path::Path &path);

typedef const std::function<const Ref(const Ref &ref, const std::string &value)>
    Binder;

static const Ref FailOnBinding(const Ref &ref, const std::string &value) {
  LOG(FATAL) << "Binding not allowed: " << ref << " <- " << value;
  return {}; // unreachable
};

bool IsReference(const path::Path &path_part);
const path::Path CanonicalizeRefs(const path::Path &path);
const RefSet RefSetFromPath(const path::Path &path);
const path::Path BindRefs(const path::Path &path, Binder binder);

} // namespace utils

} // namespace path

std::ostream &operator<<(std::ostream &os, const struct stat &s);
std::ostream &operator<<(std::ostream &os, const path::Node &n);

#endif
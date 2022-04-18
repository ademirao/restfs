#ifndef PATHS_H
#define PATHS_H

#include "logger.h"

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
using Value = std::string;
using Ref = Reference;
using ParamName = std::string;
using ReferenceSet = std::unordered_set<path::Ref>;
using RefValueMap = std::unordered_map<path::Ref, Value>;
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

struct DataImpl {
  const Path path_;
  const struct stat stat_;
  const void *data_;
 // const size_t data_size_;
};

class Node final {
public:
  Node(const Node &node) : data_(node.data_), children_(node.children_) {}
  Node(const DataImpl &data) : data_(data) {}

  const Path &path() const { return data_.path_; }

  const struct stat &stat() const { return data_.stat_; }

  template <typename Data> const Data *data() const {
    return static_cast<const Data *>(data_.data_);
  }

  //const size_t data_size() { return data_.data_size_; }
  std::vector<const path::Node *> *mutable_children() { return &children_; }

  const std::vector<const path::Node *> &children() const { return children_; }

private:
  const DataImpl data_;
  std::vector<const path::Node *> children_;
};

typedef __mode_t NodeMode;
path::Node SimpleFileNode(const path::Path &path, const void *data,
                          const size_t data_size,
                          const std::initializer_list<NodeMode> &modes);
template <typename Data>
path::Node SimpleFileNode(const path::Path &path, const Data *data,
                          const std::initializer_list<NodeMode> &modes) {
  return SimpleFileNode(path, data, sizeof *data, modes);
}

path::Node DirNode(const path::Path &key, const void *data);

namespace utils {
const std::vector<path::Path> all_prefixes(const path::Path &path);

typedef const std::function<const Ref(const Ref &ref, const std::string &value)>
    Binder;

inline const Ref FailOnBinding(const Ref &ref, const std::string &value) {
  LOG(FATAL) << "Binding not allowed: " << ref << " <- " << value;
  return {}; // unreachable
};

bool IsReference(const path::Path &path_part);
bool HasReference(const path::Path &path_part);
const path::Path ValuedPath(const path::Path &pat);

const Binder ValueBinder = [](const Reference &ref,
                              const Value &value) -> const Reference {
  return value;
};

const Binder ReferenceBinder = [](const Reference &ref,
                                  const Value &value) -> const Reference {
  return "{" + ref + "}";
};
const path::Path PathToRefValueMap(const path::Path &pat,
                                   RefValueMap *ref_value_map = nullptr);
const RefSet RefSetFromPath(const path::Path &path);
const path::Path BindRefs(const path::Path &path, Binder binder);

} // namespace utils

} // namespace path

std::ostream &operator<<(std::ostream &os, const struct stat &s);
std::ostream &operator<<(std::ostream &os, const path::Node &n);

#endif
#include "openapi.h"
#include <unistd.h>
#include <unordered_map>

std::ostream &operator<<(std::ostream &os,
                         const openapi::PathToNodeMap &path_to_node_map) {
  os << "{" << std::endl;
  for (auto it = path_to_node_map.begin(); it != path_to_node_map.end(); ++it) {
    os << it->first << ":" << it->second << std::endl;
    CHECK(it->second.stat().st_gid == getgid());
  }
  os << "}" << std::endl;
  return os;
}

std::ostream &operator<<(std::ostream &os, const openapi::Directory &d) {
  os << "Root: " << d.root() << std::endl;
  os << "Map: " << d.path_to_node_map() << std::endl;
  return os;
}

namespace openapi {

std::unique_ptr<const Directory>
NewDirectoryFromJsonValue(const std::string &host,
                          std::unique_ptr<const Json::Value> json_data) {
  PathToNodeMap path_to_node_map;
  auto insert_it =
      path_to_node_map.emplace(path::Path("/"), path::DirNode("/", nullptr));
  CHECK(insert_it.second);
  path::Node *root = &insert_it.first->second;
  const Json::Value paths = (*json_data)["paths"];
  for (auto it = paths.begin(), end = paths.end(); it != end; ++it) {
    path::Node *parent = root;
    auto current_path = parent->path();
    for (const path::Path &part : path::Path(it.key().asString())) {
      current_path /= part;
      auto insert_pair = path_to_node_map.emplace(path::Path(current_path),
                                                  path::DirNode(part, nullptr));
      if (insert_pair.second) {
        parent->mutable_children()->push_back(&insert_pair.first->second);
      }
      parent = &insert_pair.first->second;
    }
    current_path /= "res.json";
    const Json::Value *json_value = &(*it);
    auto insert_file_pair = path_to_node_map.emplace(
        current_path, path::FileNode(current_path.filename(), json_value));
    if (insert_file_pair.second) {
      parent->mutable_children()->push_back(&insert_file_pair.first->second);
    }
  }
  return std::make_unique<Directory>(host, std::move(path_to_node_map),
                                     std::move(json_data));
}

const Json::Value JsonValueFromPath(const path::Path &path) {
  Json::Value val;
  path::utils::BindRefs(path,
                        [&val](const path::Ref &ref,
                               const std::string &value) -> const path::Ref {
                          val[ref] = value;
                          return ref;
                        });
  return std::move(val);
}

PathToNodeMap::const_iterator Directory::find(const std::string &path) const {
  auto canonical_path = path::utils::CanonicalizeRefs(path);
  const Json::Value from_path = JsonValueFromPath(path);
  return path_to_node_map_.find(canonical_path);
}

} // namespace openapi

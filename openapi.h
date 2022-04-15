#ifndef OPENAPI_H
#define OPENAPI_H

#include "path.h"

#include "rest.h"
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tag_and_trait.hpp>
#include <ext/pb_ds/trie_policy.hpp>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

namespace openapi {

namespace pb_ds = __gnu_pbds;

class Directory;

} // namespace openapi

std::ostream &operator<<(std::ostream &os, const openapi::Directory &d);

namespace openapi {

using PathToNodeMap = std::map<path::Path, path::Node>;

class Directory;

Directory
NewDirectoryFromJsonValue(const std::string &host_name,
                          std::unique_ptr<const Json::Value> json_data);
const Json::Value JsonValueFromPath(const path::Path &path);

struct Entity final {
  const path::Path path;
  const path::Path read_path;
  const path::Path write_path;
  const path::Path delete_path;
  const path::NodeMode modes;
};

class Directory final {
public:
  Directory(const std::string &directory_url_prefix,
            PathToNodeMap path_to_node_map,
            const std::vector<Entity> &&entities,
            std::unique_ptr<const Json::Value> value)
      : directory_url_prefix_(directory_url_prefix),
        path_to_node_map_(std::move(path_to_node_map)),
        entities_(std::move(entities)), value_(std::move(value)) {}

  PathToNodeMap::const_iterator find(const path::Path &path) const;

  PathToNodeMap::const_iterator begin() const {
    return path_to_node_map_.begin();
  }

  PathToNodeMap::const_iterator end() const { return path_to_node_map_.end(); }

  const std::string &directory_url_prefix() const {
    return directory_url_prefix_;
  }

  operator std::string() const {
    std::stringstream ss;
    ss << root();
    return ss.str();
  }

  const path::Node &root() const {
    auto root_it = path_to_node_map_.find("/");
    CHECK(root_it != path_to_node_map_.end())
    return root_it->second;
  }

  const openapi::PathToNodeMap path_to_node_map() const {
    return path_to_node_map_;
  }

private:
  const std::string directory_url_prefix_;
  const openapi::PathToNodeMap path_to_node_map_;
  const std::vector<Entity> entities_;
  const std::unique_ptr<const Json::Value> value_;
};

} // namespace openapi

#endif
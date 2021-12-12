#include "openapi.h"
#include <unistd.h>

std::ostream &operator<<(std::ostream &os, const path::PathInfoTrie &root) {
  os << "{" << std::endl;
  for (auto it = root.begin(); it != root.end(); ++it) {
    os << it->first << ":" << *it->second << std::endl;
    CHECK(it->second->stat.st_gid == getgid());
  }
  os << "}" << std::endl;
  return os;
}
std::ostream &operator<<(std::ostream &os, const openapi::Directory &d) {
  os << d.root();
  return os;
}

namespace openapi {

std::unique_ptr<const Directory>
NewDirectoryFromJsonValue(const Json::Value &json_data) {
  auto root = std::make_unique<path::PathInfoTrie>();
  std::vector<std::unique_ptr<const path::Info>> infos;
  const Json::Value paths = json_data["paths"];
  for (auto it = paths.begin(), end = paths.end(); it != end; ++it) {
    const Json::Value path = *it;
    std::string path_str = it.key().asString();
    std::string file_path = path_str + "/res.json";
    const auto &operations = rest::constants::operations(path);
    if (!operations.empty()) {
      infos.push_back(path::FileInfo(file_path, operations));
      root->insert(std::make_pair(path::Path(file_path), infos.back().get()));
    }

    for (const path::Path &prefix : path::utils::all_prefixes(path_str)) {
      auto stat_it = root->find(prefix);
      if (stat_it != root->end()) {
        continue;
      }

      infos.push_back(path::DirInfo(prefix));
      root->insert(std::make_pair(prefix, infos.back().get()));
    }
  }

  return std::make_unique<Directory>(std::move(root), std::move(infos));
}

} // namespace openapi

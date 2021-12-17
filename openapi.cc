#include "openapi.h"
#include <unistd.h>

std::ostream &operator<<(std::ostream &os, const openapi::PathInfoTrie &root) {
  os << "{" << std::endl;
  for (auto it = root.begin(); it != root.end(); ++it) {
    os << it->first << ":" << *it->second << std::endl;
    CHECK(it->second->stat.st_gid == getgid());
  }
  os << "}" << std::endl;
  return os;
}

namespace openapi {

std::unique_ptr<const Directory>
NewDirectoryFromJsonValue(const std::string &host,
                          std::unique_ptr<const Json::Value> json_data) {
  auto root = std::make_unique<PathInfoTrie>();
  std::vector<std::unique_ptr<const path::Info>> infos;
  const Json::Value paths = (*json_data)["paths"];
  for (auto it = paths.begin(), end = paths.end(); it != end; ++it) {
    const Json::Value *json_value = &(*it);
    std::string path_str = it.key().asString();
    std::string file_path = path_str + "/res.json";
    infos.push_back(path::FileInfo(file_path, json_value));
    root->insert(std::make_pair(path::Path(file_path), infos.back().get()));

    // for (const path::Path &prefix : path::utils::all_prefixes(path_str)) {
    //   const auto stat_it = root->find(prefix);
    //   if (stat_it != root->end()) {
    //     continue;
    //   }

    //   bool ends_in_reference =
    //       regex_search(prefix.filename().c_str(), FIELD_MATCH);
    //   if (ends_in_reference) {
    //     infos.push_back(path::RefInfo(prefix, prefix.filename(), nullptr));
    //   } else {
    //     infos.push_back(path::DirInfo(prefix));
    //   }
    //   root->insert(std::make_pair(prefix, infos.back().get()));
    // }
  }

  return std::make_unique<Directory>(host, std::move(json_data),
                                     std::move(root), std::move(infos));
}

std::pair<PathInfoTrie::const_iterator, PathInfoTrie::const_iterator>
Directory::prefix_range(const std::string &path) const {
  auto canonical_path = path::utils::CanonicalizeRefs(path);

  // Aqui temos que fazer:
  // 1) Canonicalizar as referencias, removendo suas atribuicoes <ref>=<value>
  // 2) Extrair os binds. <ref>=<value>, incluindo os que vem via side channel (stdin)
  // Note to self: Nao é necessário fazer wrap dos iteradores pq os binds sao os mesmos em todo o range.
  LOG(INFO) << canonical_path;
  auto pair = root_->prefix_range(canonical_path);
  LOG(INFO) << (pair.first == pair.second);
  return pair;
}

PathInfoTrie::const_iterator Directory::find(const std::string &path) const {
  auto canonical_path = path::utils::CanonicalizeRefs(path);
  LOG(INFO) << canonical_path;
  auto it = root_->find(canonical_path);
  LOG(INFO) << (it == root_->end());
  return it;
}

} // namespace openapi

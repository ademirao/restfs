#ifndef OPENAPI_H
#define OPENAPI_H

#include "path.h"

#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tag_and_trait.hpp>
#include <ext/pb_ds/trie_policy.hpp>
#include <memory>
#include <vector>
namespace openapi {

namespace pb_ds = __gnu_pbds;

typedef pb_ds::trie<std::string, const path::Info *,
                    pb_ds::trie_string_access_traits<>, pb_ds::pat_trie_tag,
                    pb_ds::trie_prefix_search_node_update>
    PathInfoTrie;

} // namespace openapi

std::ostream &operator<<(std::ostream &os, const openapi::PathInfoTrie &d);

namespace openapi {

class Directory;

std::unique_ptr<const Directory>
NewDirectoryFromJsonValue(const std::string &host_name,
                          std::unique_ptr<const Json::Value> json_data);

class Directory final {
public:
  Directory(const std::string &directory_url_prefix,
            std::unique_ptr<const Json::Value> value,
            std::unique_ptr<const PathInfoTrie> root,
            std::vector<std::unique_ptr<const path::Info>> infos)
      : directory_url_prefix_(directory_url_prefix), value_(std::move(value)),
        root_(std::move(root)), infos_(std::move(infos)) {}

  PathInfoTrie::const_iterator find(const std::string &path) const;

  PathInfoTrie::const_iterator begin() const { return root_->begin(); }

  PathInfoTrie::const_iterator end() const { return root_->end(); }

  std::pair<PathInfoTrie::const_iterator, PathInfoTrie::const_iterator>
  prefix_range(const std::string &path) const;
  
  const std::string &directory_url_prefix() const {
    return directory_url_prefix_;
  }
  const std::vector<std::unique_ptr<const path::Info>> &infos() const {
    return infos_;
  }
  
  operator std::string() const {
    std::stringstream ss;
    ss << *root_;
    return ss.str();
  }

private:
  const std::string directory_url_prefix_;
  const std::unique_ptr<const Json::Value> value_;
  const std::unique_ptr<const PathInfoTrie> root_;
  const std::vector<std::unique_ptr<const path::Info>> infos_;
};

} // namespace openapi

#endif
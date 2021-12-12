#ifndef PATHS_H
#define PATHS_H

#include "logger.h"
#include "rest.h"

#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tag_and_trait.hpp>
#include <ext/pb_ds/trie_policy.hpp>
#include <filesystem>
#include <sys/stat.h>

namespace path {

typedef std::filesystem::path Path;

namespace pb_ds = __gnu_pbds;

class Info final {
public:
  Info(const Info &info) : stat(info.stat) {}
  Info(const struct stat &s, std::vector<rest::constants::OPERATIONS> ops = {})
      : stat(s) {}

  const struct stat stat;
};

typedef pb_ds::trie<std::string, const Info *,
                    pb_ds::trie_string_access_traits<>, pb_ds::pat_trie_tag,
                    pb_ds::trie_prefix_search_node_update>
    PathInfoTrie;

std::unique_ptr<const path::Info>
FileInfo(const std::string &key,
         const std::vector<rest::constants::OPERATIONS> &operations);
std::unique_ptr<const path::Info> DirInfo(const std::string &key);

namespace utils {
std::vector<path::Path> all_prefixes(const path::Path &path);
} // namespace utils

} // namespace path

std::ostream &operator<<(std::ostream &os, const path::Info &s);
std::ostream &operator<<(std::ostream &os, const struct stat &s);

#endif
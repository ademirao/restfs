#define FUSE_USE_VERSION 35

#include "path.h"
#include "rest.h"

#include <functional>
#include <regex>
#include <string>
#include <unistd.h>

#define LOG_FIELD(OBJ, FIELD) #FIELD << ": " << (OBJ).FIELD << ", "

std::ostream &operator<<(std::ostream &os, const path::Info &s) {
  return os << s.stat;
}

std::ostream &operator<<(std::ostream &os, const struct stat &s) {
  return os << "stat{" << LOG_FIELD(s, st_dev) << LOG_FIELD(s, st_ino)
            << LOG_FIELD(s, st_mode) << LOG_FIELD(s, st_nlink)
            << LOG_FIELD(s, st_uid)
            << LOG_FIELD(s, st_gid)     /* Group ID of owner */
            << LOG_FIELD(s, st_rdev)    /* Device ID (if special file) */
            << LOG_FIELD(s, st_size)    /* Total size, in bytes */
            << LOG_FIELD(s, st_blksize) /* Block size for filesystem I/O */
            << LOG_FIELD(s, st_blocks)  /* Number of 512B blocks allocated */
            << "}";
}

namespace path {
std::unique_ptr<const path::Info> FileInfo(const std::string &key,
                                           const Json::Value *json) {

  struct stat s;
  s.st_dev = 0;
  s.st_ino = std::hash<std::string>{}(key); /* Inode number */
  s.st_uid = getuid();                      /* User ID of owner */
  s.st_gid = getgid();                      /* Group ID of owner */
  s.st_mode = S_IFREG | 0000;               /* File type and mode */
  for (const auto op : rest::constants::operations(*json)) {
    switch (op) {
    case rest::constants::HEAD:
    case rest::constants::GET:
      s.st_mode |= S_IREAD;
      break;
    case rest::constants::PATCH:
    case rest::constants::DELETE:
    case rest::constants::POST:
    case rest::constants::PUT:
      s.st_mode |= S_IWRITE;
      break;
    case rest::constants::INVALID:
    case rest::constants::TRACE:
    case rest::constants::OPTIONS:
      LOG(FATAL) << "Unsupported operation: " << op;
      break;
    }
  }
  s.st_nlink = 0;   /* Number of hard links */
  s.st_rdev = 0;    /* ID of device containing file */
  s.st_size = 0;    /* Total size, in bytes */
  s.st_blksize = 0; /* Block size for filesystem I/O */
  s.st_blocks = 0;  /* Number of 512B blocks allocated */
  return std::make_unique<path::Info>(s, json);
}

std::unique_ptr<const path::Info> DirInfo(const std::string &key,
                                          const Json::Value *json) {
  struct stat s;
  s.st_dev = 0;
  s.st_ino = std::hash<std::string>{}(key); /* Inode number */
  s.st_uid = getuid();                      /* User ID of owner */
  s.st_gid = getgid();                      /* Group ID of owner */
  s.st_mode = S_IFDIR | 0755;               /* File type and mode */
  s.st_nlink = 0;                           /* Number of hard links */
  s.st_rdev = 0;                            /* ID of device containing file */
  s.st_size = 0;                            /* Total size, in bytes */
  s.st_blksize = 0;                         /* Block size for filesystem I/O */
  s.st_blocks = 0; /* Number of 512B blocks allocated */
  return std::make_unique<path::Info>(s, json);
}

namespace utils {
const std::vector<path::Path> all_prefixes(const path::Path &path) {
  std::vector<path::Path> prefixes;
  path::Path prefix = "/";
  for (const path::Path &part : path) {
    prefix.append(part.u32string());
    prefixes.push_back(prefix);
  }
  return prefixes;
}

const path::Path CanonicalizeRefs(const path::Path &path) {
  return BindRefs(path,
                  [](const ParamReference &ref, const std::string &value)
                      -> const ParamReference { return ref; });
}

const path::Path BindRefs(const path::Path &path, Binder binder) {
  path::Path new_path;
  LOG(INFO) << "Path: " << path;
  for (path::Path::const_iterator it = path.begin(), end = path.end();
       it != end; ++it) {
    const std::string it_str = *it;
    const size_t it_str_len = it_str.length();
    if (it_str_len == 0) {
      continue;
    }

    if (it_str[0] != '{') {
      new_path /= *it;
      continue;
    }

    if (it_str.back() != '}') {
      new_path /= *it;
      continue;
    }

    auto ref_bind = it_str.substr(1, it_str.length() - 2);
    auto eq_char_pos = ref_bind.find('=');
    std::pair<std::string, std::string> canonical_ref_value =
        (eq_char_pos != ref_bind.npos)
            ? std::make_pair(ref_bind.substr(0, eq_char_pos),
                             ref_bind.substr(eq_char_pos + 1))
            : std::make_pair(ref_bind, "");

    new_path /=
        /* append */ path::Path(
            '{' +
            binder(canonical_ref_value.first, canonical_ref_value.second) +
            '}');
  }
  return new_path;
} // namespace utils

const ::path::RefSet RefSetFromPath(const path::Path &path) {
  typedef std::unordered_set<ParamReference> ParamRefSet;
  ParamRefSet ref_set;
  BindRefs(path,
           [&ref_set](const ParamReference &ref,
                      const std::string &value) -> const ParamReference {
             ref_set.insert(ref);
             return ref;
           });
  return ref_set;
}
} // namespace utils
} // namespace path

#define FUSE_USE_VERSION 35

#include "path.h"

#include <cmath>
#include <functional>
#include <regex>
#include <string>
#include <unistd.h>

#define LOG_FIELD(OBJ, FIELD) #FIELD << ": " << (OBJ).FIELD << ", "

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

std::ostream &operator<<(std::ostream &os, const path::Node &n) {

  os << n.path() << ": [";
  for (const auto &child : n.children()) {
    os << *child << ",";
  }
  os << "]";
  return os;
}

namespace path {

path::Node SimpleFileNode(const path::Path &path, const void *data,
                          const size_t data_size,
                          const std::initializer_list<NodeMode> &modes) {
  struct stat s {};
  s.st_dev = 0;
  s.st_ino = std::hash<path::Path>{}(path); /* Inode number */
  s.st_uid = getuid();                      /* User ID of owner */
  s.st_gid = getgid();                      /* Group ID of owner */
  s.st_mode = S_IFREG | 0000;               /* File type and mode */
  for (const NodeMode mode : modes) {
    s.st_mode |= mode;
  }
  s.st_nlink = 0;           /* Number of hard links */
  s.st_rdev = 0;            /* ID of device containing file */
  s.st_size = data_size;    /* Total size, in bytes */
  s.st_blksize = s.st_size; /* Block size for filesystem I/O */
  s.st_blocks =
      std::ceil(s.st_size / 512); /* Number of 512B blocks allocated */
  return path::Node({path, s, data, data_size});
}

path::Node DirNode(const path::Path &path, const void *data) {
  struct stat s {};
  s.st_dev = 0;
  s.st_ino = std::hash<std::string>{}(path); /* Inode number */
  s.st_uid = getuid();                       /* User ID of owner */
  s.st_gid = getgid();                       /* Group ID of owner */
  s.st_mode = S_IFDIR | 0755;                /* File type and mode */
  s.st_nlink = 0;                            /* Number of hard links */
  s.st_rdev = 0;                             /* ID of device containing file */
  s.st_size = 0;                             /* Total size, in bytes */
  s.st_blksize = 0;                          /* Block size for filesystem I/O */
  s.st_blocks = 0; /* Number of 512B blocks allocated */
  return path::Node({path, s, data, 0});
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

const path::Path PathToRefValueMap(const path::Path &path,
                                   RefValueMap *ref_value_map) {
  return BindRefs(
      path,
      (ref_value_map == nullptr)
          ? ReferenceBinder
          : [ref_value_map](const Reference &ref,
                            const Value &value) -> const Reference {
        const auto insert_it = ref_value_map->emplace(ref, value);

        if (!insert_it.second) { // not inserted
          CHECK_M(insert_it.first->second.empty(),
                  "Duplicated reference: " + insert_it.first->first + ", "
                                           + insert_it.first->second
                                           + " trying assign " + value);
          insert_it.first->second = value;
          LOG(INFO) << "Found duplicated reference with null value. Assigned: "
                    << insert_it.first->second;
        }
        return ReferenceBinder(ref, value);
      });
}

bool IsReference(const path::Path &path) {
  const std::string &filename = path.filename();
  return filename.length() > 0 && filename[0] == '{';
}

bool HasReference(const path::Path &path) {
  for (const path::Path &part : path) {
    if (IsReference(part))
      return true;
  }
  return false;
}

const path::Path BindRefs(const path::Path &path, Binder binder) {
  path::Path new_path;
  for (path::Path::const_iterator it = path.begin(), end = path.end();
       it != end; ++it) {
    if (!IsReference(*it)) {
      new_path /= *it;
      continue;
    }
    const std::string it_str = *it;
    const size_t ref_end_pos = it_str.find('}');
    bool has_ref_end = ref_end_pos != it_str.npos;
    auto ref_bind = it_str.substr(1, ref_end_pos - ((has_ref_end) ? 1 : 0));
    auto eq_char_pos = ref_bind.find(':');
    std::pair<std::string, std::string> canonical_ref_value =
        (eq_char_pos != ref_bind.npos)
            ? std::make_pair(ref_bind.substr(0, eq_char_pos),
                             ref_bind.substr(eq_char_pos + 1))
            : std::make_pair(ref_bind, "");

    new_path /=
        /* append */ path::Path(
            binder(canonical_ref_value.first, canonical_ref_value.second) +
            it_str.substr(ref_end_pos + 1));
  }
  return new_path;
} // namespace utils

const ::path::RefSet RefSetFromPath(const path::Path &path) {
  RefSet ref_set;
  BindRefs(path,
           [&ref_set](const Reference &ref,
                      const std::string &value) -> const Reference {
             ref_set.insert(ref);
             return ref;
           });
  return ref_set;
}
} // namespace utils
} // namespace path

#define FUSE_USE_VERSION 35

#include "path.h"
#include "rest.h"

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
std::unique_ptr<const path::Info>
FileInfo(const std::string &key,
         const std::vector<rest::constants::OPERATIONS> &operations) {

  struct stat s;
  s.st_dev = 0;
  s.st_ino = std::hash<std::string>{}(key); /* Inode number */
  s.st_uid = getuid();                      /* User ID of owner */
  s.st_gid = getgid();                      /* Group ID of owner */
  s.st_mode = S_IFREG | 0000;               /* File type and mode */
  for (const auto op : operations) {
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
  return std::make_unique<path::Info>(s);
}

std::unique_ptr<const path::Info> DirInfo(const std::string &key) {
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
  return std::make_unique<path::Info>(s);
}

namespace utils {
std::vector<path::Path> all_prefixes(const path::Path &path) {
  std::vector<path::Path> prefixes;
  path::Path prefix = "/";
  for (const path::Path &part : path) {
    prefix.append(part.u32string());
    prefixes.push_back(prefix);
  }
  return prefixes;
}
} // namespace utils

} // namespace path
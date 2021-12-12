#define FUSE_USE_VERSION 35

#include "http.h"
#include "logger.h"
#include "openapi.h"
#include "path.h"
#include "rest.h"

#include <curl/curl.h>
#include <filesystem>
#include <fuse3/fuse.h>
#include <iostream>
#include <map>
#include <string>
#include <unordered_set>
#include <unistd.h>

std::unique_ptr<const openapi::Directory> directory;

int api_getattr(const char *path, struct stat *stat,
                struct fuse_file_info *fi) {
  LOG(INFO) << "api_get_attr: " << path;
  auto found = directory->root().find(path);
  if (found == directory->root().end()) {
    return -1;
  }
  *stat = found->second->stat;
  stat->st_atime = time(nullptr);
  stat->st_mtime = time(nullptr);
  return 0;
}

int api_read(const char *path, char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi) {
  LOG(INFO) << "api_read " << path << ", " << std::string(buf, size) << ", "
            << offset << ", " << fi;
  return 0;
}

int api_write(const char *path, const char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) {
  LOG(INFO) << "api_write " << path << ", " << std::string(buf, size) << ", "
            << offset << ", " << fi;
  return 0;
}

int api_statfs(const char *path, struct statvfs *statv) {
  LOG(INFO) << "api_statfs " << path << ", " << statv;
  return 0;
}

int api_readdir(const char *path_str, void *buf, fuse_fill_dir_t filler,
                off_t offset, struct fuse_file_info *fi,
                enum fuse_readdir_flags flag) {
  LOG(INFO) << "api_readdir " << path_str << ", " << filler << ", " << offset
            << ", " << fi;
  const ::path::Path path(path_str);
  // Substituia por uma utilizacao mais eficiente de prefix range. Nao
  // precisamos descer a arvore toda. Precisamos apenas dos filhos imediatos.
  auto it_pair = directory->root().prefix_range(path);
  std::unordered_set<std::string> already_reported;
  for (auto it = it_pair.first; it != it_pair.second; ++(it)) {
    const path::Path sub_path(it->first);
    const path::Path relative_sub_path(
        std::filesystem::relative(sub_path, path));

    if (relative_sub_path.empty()) {
      continue;
    }

    const std::string &sub_path_first_part = *(relative_sub_path.begin());

    if (already_reported.find(sub_path_first_part) != already_reported.end()) {
      continue;
    }

    if (filler(buf, sub_path_first_part.c_str(), &(it->second->stat), 0,
               (fuse_fill_dir_flags)0)) {
      return -1;
    }
    already_reported.insert(sub_path_first_part);
  }
  return 0; // Tell Fuse we're done.
}

static int api_readlink(const char *path, char *buf, size_t size) {
  LOG(INFO) << "api_readlink " << path << ", " << std::string(buf, size);
  return 0;
}

void *api_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
  cfg->direct_io = 1;

  http::Request request;
  static const std::string HTTP_PREFIX = "http";
  const std::string api_spec_path = getenv("API_SPEC");
  Json::Value json_data;
  if (api_spec_path.substr(HTTP_PREFIX.length()) == HTTP_PREFIX) {
    const auto response = request.fetch(api_spec_path);
    CHECK(response->http_code == 200);
    response->data >> json_data;
  } else {
    auto file = open(api_spec_path.c_str(), S_IRUSR);
    static char BUFFER[256];
    ssize_t size = read(file, &BUFFER, 256);
    LOG(FATAL) << std::string(BUFFER, size);
    return 0;
  }

  directory = openapi::NewDirectoryFromJsonValue(json_data);
  LOG(INFO) << *directory;
  return 0;
}

int api_truncate(const char *path, off_t off, struct fuse_file_info *fi) {
  LOG(INFO) << "api_truncate " << path << ", " << off;
  return 0;
}

int main(int argc, char *argv[]) {
  struct fuse_operations fuse = {
      .getattr = api_getattr,
      .readlink = api_readlink,
      .truncate = api_truncate,
      .read = api_read,
      .write = api_write,
      .statfs = api_statfs,
      .readdir = api_readdir,
      .init = api_init,
  };

  return fuse_main(argc, argv, &fuse, nullptr);
}

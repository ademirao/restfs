#define FUSE_USE_VERSION 35

#include "http.h"
#include "logger.h"
#include "openapi.h"
#include "path.h"
#include "rest.h"

#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <fuse3/fuse.h>
#include <iostream>
#include <map>
#include <string.h>
#include <string>
#include <unistd.h>
#include <unordered_set>

std::unique_ptr<const openapi::Directory> directory;

int api_getattr(const char *path, struct stat *stat,
                struct fuse_file_info *fi) {
  LOG(INFO) << "api_get_attr: " << path;
  auto found = directory->find(path);
  if (found == directory->end()) {
    LOG(INFO) << "NOT FOUND!";
    return -ENOENT;
  }
  *stat = found->second.stat();
  return 0;
}

int api_read(const char *in_path, char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi) {
  LOG(INFO) << "api_read " << in_path;
  http::Request request;
  const std::filesystem::path path =
      std::filesystem::path(in_path).parent_path();

  auto response =
      request.fetch(directory->directory_url_prefix() + path.string());
  const std::string filecontent = response->data.str();
  if (response->http_code != 200) {
    return -ENOENT;
  }
  size_t len = filecontent.length();
  if (offset >= len) {
    return 0;
  }

  if (offset + size > len) {
    memcpy(buf, filecontent.c_str() + offset, len - offset);
    return len - offset;
  }

  memcpy(buf, filecontent.c_str() + offset, size);
  return -ENOENT;
}

int api_write(const char *path, const char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) {
  LOG(INFO) << "api_write " << path << ", "
            << ", " << offset << ", " << fi;
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
  const path::Path path(path::utils::CanonicalizeRefs(path_str));
  const path::Path &filename(path.filename());

  std::unordered_set<std::string> already_reported;

  // Substituia por uma utilizacao mais eficiente de prefix range. Nao
  // precisamos descer a arvore toda. Precisamos apenas dos filhos imediatos.
  auto it = directory->find(path);
  if (it == directory->end()) {
    LOG(INFO) << "Not found";
    return -ENOENT;
  }
  LOG(INFO) << it->second;

  for (auto child : it->second.children()) {
    LOG(INFO) << *child;
    if (filler(buf, child->path().filename().c_str(), &(child->stat()), 0,
               (fuse_fill_dir_flags)0)) {
      return -1;
    }
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
  auto json_data = std::make_unique<Json::Value>();
  const std::string prefix = api_spec_path.substr(0, HTTP_PREFIX.length());
  if (prefix == HTTP_PREFIX) {
    const auto response = request.fetch(api_spec_path);
    CHECK(response->http_code == 200);
    response->data >> *json_data;
  } else {
    std::ifstream stream;
    stream.open(api_spec_path.c_str());
    if (!stream.is_open()) {
      LOG(FATAL) << "Failed to open: " << api_spec_path << "";
    }
    std::stringstream buffer;
    buffer << stream.rdbuf();
    buffer >> *json_data;
  }

  const std::string &api_addr = getenv("API_ADDR");
  directory =
      openapi::NewDirectoryFromJsonValue(api_addr, std::move(json_data));

  LOG(INFO) << "Loaded dir:" << *directory;

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

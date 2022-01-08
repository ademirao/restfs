#define FUSE_USE_VERSION 36

#include "http.h"
#include "logger.h"
#include "openapi.h"
#include "path.h"
#include "rest.h"

#include <algorithm>
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

inline bool ends_with(const std::string &value, const std::string &ending) {
  if (ending.size() > value.size())
    return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

int str_to_buffer(const std::string &content, char *buf, size_t size,
                  off_t offset) {
  const size_t len = content.length();

  if (offset >= (off_t)len) {
    return 0;
  }

  if (offset + size > len) {
    memcpy(buf, content.c_str() + offset, len - offset);
    return len - offset;
  }

  memcpy(buf, content.c_str() + offset, size);
  return size;
}

const std::string ReadMetadataNode(const path::Path &path,
                                   const path::Node &node) {
  const Json::Value *v = node.data<Json::Value>();
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "  "; // assume default for comments is None
  return Json::writeString(builder, v);
}

const std::string ReadEntityNode(const path::Path &path,
                                 const path::Node &node) {
  const openapi::Entity *v = node.data<openapi::Entity>();
  return v->path.string();
}

const std::string ReadOperationNode(const path::Path &path,
                                    const path::Node &node) {
  const path::Path filestem = node.path().filename().stem();
  const std::string operation_str =
      (filestem.has_extension()) ? filestem.extension() : filestem.stem();
  const auto find_it = rest::constants::operations_map().find(operation_str);
  if (find_it == rest::constants::operations_map().end()) {
    LOG(INFO) << "Unexpected file name";
    return "";
  }
  const path::Path value_path =
      path::utils::BindRefs(path, path::utils::ValueBinder);

  const http::Response response = http::Request(find_it->second)
                                      .fetch(directory->directory_url_prefix() +
                                             value_path.parent_path().string());
  if (response.http_code != 200) {
    return "";
  }
  return response.data.str();
}

int api_read(const char *in_path, char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi) {
  LOG(INFO) << "api_read " << in_path;
  const path::Path ref_path =
      path::utils::BindRefs(in_path, path::utils::ReferenceBinder);
  const auto it = directory->find(ref_path);
  if (it == directory->end()) {
    return -ENOENT;
  }
  const path::Path path = it->first;
  if (ends_with(path.filename().string(), "metadata.json")) {
    return str_to_buffer(ReadMetadataNode(path, it->second), buf, size, offset);
  }

  if (ends_with(path.filename().string(), "entity.json")) {
    return str_to_buffer(ReadEntityNode(path, it->second), buf, size, offset);
  }

  return str_to_buffer(ReadOperationNode(path, it->second), buf, size, offset);
}

int api_write(const char *in_path, const char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) {
  const path::Path ref_path =
      path::utils::BindRefs(in_path, path::utils::ReferenceBinder);
  const auto it = directory->find(ref_path);
  if (it == directory->end()) {
    return -ENOENT;
  }

  return size;
}

int api_statfs(const char *path, struct statvfs *statv) {
  LOG(INFO) << "api_statfs " << path << ", " << statv;
  return 0;
}

int api_readdir(const char *path_str, void *buf, fuse_fill_dir_t filler,
                off_t offset, struct fuse_file_info *fi,
                enum fuse_readdir_flags flag) {
  const path::Path path(path::utils::PathToRefValueMap(path_str));
  const path::Path &filename(path.filename());

  auto it = directory->find(path);
  if (it == directory->end()) {
    return -ENOENT;
  }

  for (auto child : it->second.children()) {
    if (filler(buf, child->path().filename().c_str(), &(child->stat()), 0,
               (fuse_fill_dir_flags)0)) { // Error filling the buffer.
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
    http::Response response = request.fetch(api_spec_path);
    CHECK(response.http_code == 200);
    response.data >> *json_data;
  } else {
    std::ifstream stream;
    stream.open(api_spec_path.c_str());
    if (!stream.is_open()) {
      LOG(FATAL) << "Failed to open: " << api_spec_path << "";
      return nullptr; // unreachable
    }
    std::stringstream buffer;
    buffer << stream.rdbuf();
    buffer >> *json_data;
  }

  const std::string &api_addr = getenv("API_ADDR");
  directory =
      openapi::NewDirectoryFromJsonValue(api_addr, std::move(json_data));
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

  LOG(INFO) << "CREATED";
  auto err = fuse_main(argc, argv, &fuse, nullptr);
  LOG(INFO) << "FUSE FUDEU " << err;
  return err;
}

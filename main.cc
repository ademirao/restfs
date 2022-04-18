#define FUSE_USE_VERSION 36

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
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
#include <sstream>
#include <string.h>
#include <string>
#include <unistd.h>
#include <unordered_set>

ABSL_FLAG(std::string, api_spec_addr, "/dev/null",
          "Address of the API spec (openapi.json). May be local path or url "
          "starting with 'http'.");

ABSL_FLAG(std::string, api_host_addr, "",
          "Address of the API spec (openapi.json).");

ABSL_FLAG(std::string, mount_location, "",
          "Comma separated list of flags to be forwarded to fuse.");

ABSL_FLAG(std::string, header_file_addr, "/dev/null",
          "List of headers to be attached");

struct PrivateContext {
  const openapi::Directory dir_;
  const http::Headers headers_;
};

const PrivateContext *private_context() {
  const PrivateContext *ctx = static_cast<const PrivateContext *>(fuse_get_context()->private_data);
  CHECK_M(ctx != nullptr, "Null private context");
  return ctx;
}

const openapi::Directory &directory() {
  return private_context()->dir_;
}

const http::Headers &headers() {
  return private_context()->headers_;
}

int api_getattr(const char *path, struct stat *stat,
                struct fuse_file_info *fi) {
  LOG(INFO) << "api_get_attr: " << path;
  auto found = directory().find(path);
  if (found == directory().end()) {
    LOG(INFO) << path << " - NOT FOUND!";
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
  const Json::Value *data = node.data<Json::Value>();
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "  "; // assume default for comments is None
  const std::string result = Json::writeString(builder, *data);
  return result;
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

  const http::Response response =
      http::Request(find_it->second, headers())
          .fetch(directory().directory_url_prefix() +
                 value_path.parent_path().string());
  if (response.http_code != 200) {
    LOG(INFO) << response.data.str();
    return "";
  }
  return response.data.str();
}

int api_read(const char *in_path, char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi) {
  LOG(INFO) << "api_read " << in_path;
  const path::Path ref_path =
      path::utils::BindRefs(in_path, path::utils::ReferenceBinder);
  const auto it = directory().find(ref_path);
  if (it == directory().end()) {
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
  const auto it = directory().find(ref_path);
  if (it == directory().end()) {
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

  auto it = directory().find(path);
  if (it == directory().end()) {
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

std::stringstream read_content(const std::string &address) {
  static const std::string HTTP_PREFIX = "http";
  http::Request request;
  const std::string prefix = address.substr(0, HTTP_PREFIX.length());
  if (prefix == HTTP_PREFIX) {
    http::Response response = request.fetch(address);
    CHECK(response.http_code == 200);
    return std::move(response.data);
  }

  std::ifstream stream;
  stream.open(address.c_str());
  CHECK_M(stream.is_open(), "Failed to open: " + address + "");
  std::stringstream buffer;
  buffer << stream.rdbuf();
  return buffer;
}

openapi::Directory LoadDirectoryFromFlags() {
  std::stringstream api_spec_stream =
      read_content(absl::GetFlag(FLAGS_api_spec_addr));
  const std::string &api_host_addr = absl::GetFlag(FLAGS_api_host_addr);

  auto json_data = std::make_unique<Json::Value>();
  api_spec_stream >> *json_data;

  return openapi::NewDirectoryFromJsonValue(api_host_addr,
                                            std::move(json_data));
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
  };

  // If the command-line contains a value for logtostderr, use that.
  // Otherwise, use the default (as set above).
  absl::ParseCommandLine(argc, argv);

  std::stringstream headers_file_stream =
      read_content(absl::GetFlag(FLAGS_header_file_addr));
  http::Headers headers;
  for (std::string line; std::getline(headers_file_stream, line);) {
    headers.AppendHeaderLine(line);
  }
  PrivateContext private_context = {
      LoadDirectoryFromFlags(),
      headers,
  };

  std::string mount_location = absl::GetFlag(FLAGS_mount_location);
  char *args[] = {argv[0] /* argv[0] = program name */, "-s", "-f",
                  const_cast<char *>(mount_location.c_str())};
  return fuse_main(sizeof(args) / sizeof(char *), args, &fuse,
                   &private_context);
}

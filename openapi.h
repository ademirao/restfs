#ifndef OPENAPI_H
#define OPENAPI_H

#include "path.h"

#include <memory>
#include <vector>

namespace openapi {

class Directory;

std::unique_ptr<const Directory>
NewDirectoryFromJsonValue(const Json::Value &json_data);

class Directory final {
public:
  Directory(std::unique_ptr<const path::PathInfoTrie> root,
            std::vector<std::unique_ptr<const path::Info>> infos)
      : root_(std::move(root)), infos_(std::move(infos)) {}

  const path::PathInfoTrie &root() const { return *root_; }
  const std::vector<std::unique_ptr<const path::Info>> &infos() const {
    return infos_;
  }

private:
  const std::unique_ptr<const path::PathInfoTrie> root_;
  const std::vector<std::unique_ptr<const path::Info>> infos_;
};

} // namespace openapi

std::ostream &operator<<(std::ostream &os, const path::PathInfoTrie &d);
std::ostream &operator<<(std::ostream &os, const openapi::Directory &d);

#endif
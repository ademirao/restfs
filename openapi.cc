#include "openapi.h"
#include <unistd.h>
#include <unordered_map>

std::ostream &operator<<(std::ostream &os,
                         const openapi::PathToNodeMap &path_to_node_map) {
  os << "{" << std::endl;
  for (auto it = path_to_node_map.begin(); it != path_to_node_map.end(); ++it) {
    os << it->first << ":" << it->second << std::endl;
    CHECK(it->second.stat().st_gid == getgid());
  }
  os << "}" << std::endl;
  return os;
}

std::ostream &operator<<(std::ostream &os, const openapi::Directory &d) {
  os << "Root: " << d.root() << std::endl;
  os << "Map: " << d.path_to_node_map() << std::endl;
  return os;
}

namespace path {
template <>
Node SimpleFileNode<Json::Value>(const path::Path &path,
                                 const Json::Value *data,
                                 const std::initializer_list<NodeMode> &modes) {
  Json::FastWriter fw;
  const std::string filecontent = fw.write(*data);
  return SimpleFileNode(path, data, filecontent.length(), modes);
}

} // namespace path

namespace openapi {

template <typename InputIt>
std::string join(InputIt first, InputIt last, const std::string &separator = "",
                 const std::string &concluder = "") {
  if (first == last) {
    return concluder;
  }

  std::stringstream ss;
  ss << *first;
  ++first;

  while (first != last) {
    ss << separator;
    ss << *first;
    ++first;
  }

  ss << concluder;

  return ss.str();
}

static const Json::Value *Find(const Json::Value &value,
                               const std::string &key) {
  return value.find(key.c_str(), key.c_str() + key.length());
}

static const Json::Value &Find(const Json::Value &value, const std::string &key,
                               const Json::Value &default_value) {
  const Json::Value *found = Find(value, key);
  return (found == nullptr) ? default_value : *found;
}

static const Json::Value &FindOrDie(const Json::Value &value,
                                    const std::string &key) {
  const Json::Value *found = Find(value, key);
  CHECK(found != nullptr);
  return *found;
}

class NodeFactory final {
public:
  NodeFactory(const Json::Value *root) : root_(root) {}
  ~NodeFactory() {}

  const Json::Value &ResolveRef(const Json::Value &value) const {
    const Json::Value &ret_value = Find(value, "$ref", Json::Value::null);
    if (ret_value == Json::Value::null) {
      return value;
    }

    const std::string &ref_value = ret_value.asString();
    CHECK(ref_value[0] == '#');
    const path::Path ref_value_path(ref_value.substr(1));
    const Json::Value *current = root_;
    for (const auto &part : ref_value_path) {
      const std::string part_str = part.string();
      if (part_str == "/")
        continue;
      const char *part_str_end = part_str.c_str() + part.string().length();
      current = current->find(part_str.c_str(), part_str_end);
      CHECK_M(current != nullptr, "Couldn't find part (" + part.string() +
                                      ") in " + ref_value_path.string());
    }
    return *current;
  }
  path::Node OperationNode(const rest::constants::OPERATIONS op,
                           const Json::Value *json) const {
    path::NodeMode node_mode = 0; /* File type and mode */
    std::vector<std::string> query_suffix;
    path::Path filename = "";
    const Json::Value &parameters = Find(*json, "parameters", Json::Value::null);
    for (const auto parameter : parameters) {
      const auto &param = ResolveRef(parameter);
      const auto &in = Find(param, "in", Json::Value::null);
      if (in.asString() == "query")
        continue;
      const auto &required_param = Find(param, "required", Json::Value::null);
      bool required =
          (required_param != Json::Value::null) && required_param.asBool();
      if (required) {
        const auto &name = Find(param, "name", Json::Value::null);
        query_suffix.push_back(name.asString());
      }
    }
    std::sort(query_suffix.begin(), query_suffix.end());

    if (!query_suffix.empty()) {
      filename +=
          "{" + join(query_suffix.begin(), query_suffix.end(), ",") + "}.";
    }
    filename += std::string(rest::constants::OPERATION_NAMES[op]) + ".json";

    switch (op) {
    case rest::constants::HEAD:
    case rest::constants::GET:
      node_mode |= S_IREAD;
      break;
    case rest::constants::PATCH:
    case rest::constants::DELETE:
    case rest::constants::POST:
    case rest::constants::PUT:
      node_mode |= S_IWRITE;
      break;
    case rest::constants::INVALID:
    case rest::constants::TRACE:
    case rest::constants::OPTIONS:
      LOG(FATAL) << "Unsupported operation: " << op;
      break;
    }

    return path::SimpleFileNode(filename, json, {node_mode});
  }

private:
  const Json::Value *root_;
};

std::unique_ptr<const Directory>
NewDirectoryFromJsonValue(const std::string &host,
                          std::unique_ptr<const Json::Value> json_data) {
  PathToNodeMap path_to_node_map;
  auto insert_it =
      path_to_node_map.emplace(path::Path("/"), path::DirNode("/", nullptr));
  CHECK(insert_it.second);
  path::Node *root = &insert_it.first->second;
  auto insert_node = [root, &path_to_node_map](const path::Path &in_path,
                                               path::Node &&node) {
    const auto path = path::utils::PathToRefValueMap(in_path);
    auto insert_pair = path_to_node_map.emplace(path, node);
    if (insert_pair.second) {
      auto parent = path_to_node_map.find(path.parent_path());
      CHECK(parent != path_to_node_map.end());
      parent->second.mutable_children()->push_back(&insert_pair.first->second);
    }
    return insert_pair;
  };
  const path::Path root_meta_json("/metadata.json");
  insert_node(root_meta_json, // concat
              path::SimpleFileNode(root_meta_json.filename(), &(*json_data),
                                   {S_IREAD}));

  const Json::Value &paths = (*json_data)["paths"];
  NodeFactory factory(json_data.get());
  for (auto it = paths.begin(), end = paths.end(); it != end; ++it) {
    const path::Path path = path::utils::PathToRefValueMap(it.key().asString());
    const Json::Value &value = *it;
    path::Node *parent = root;
    [&factory, &path, &value, &parent, &insert_node]() {
      auto current_path = parent->path();
      for (const path::Path &part : path) {
        current_path /= part;
        auto insert_pair =
            insert_node(current_path, path::DirNode(part, nullptr));
        LOG(INFO) << insert_pair.first->first << ": "
                  << insert_pair.first->second;
        parent = &insert_pair.first->second;
      }

      for (const std::string &op_name : value.getMemberNames()) {
        const auto &op_json = value[op_name];
        const auto it = rest::constants::operations_map().find(op_name);
        CHECK(it != rest::constants::operations_map().end());
        path::Node node = factory.OperationNode(it->second, &op_json);
        const auto node_path = node.path();
        insert_node(current_path / node_path, std::move(node));

        const path::Path meta_json =
            current_path /
            (node_path.filename().stem().string() + ".metadata.json");
        auto insert_pair =
            insert_node(meta_json, path::SimpleFileNode(meta_json.filename(),
                                                        &op_json, {S_IREAD}));
        LOG(INFO) << insert_pair.second << " - " << insert_pair.first->first
                  << ": " << insert_pair.first->second << "---->" << op_name;
        CHECK_M(insert_pair.second,
                "Path already exists: " + meta_json.string());
      }
    }();
  }
  return std::make_unique<Directory>(host, std::move(path_to_node_map),
                                     std::move(json_data));
}

const Json::Value JsonValueFromPath(const path::Path &path) {
  Json::Value val;
  path::utils::BindRefs(path,
                        [&val](const path::Ref &ref,
                               const std::string &value) -> const path::Ref {
                          val[ref] = value;
                          return ref;
                        });
  return std::move(val);
}

PathToNodeMap::const_iterator Directory::find(const path::Path &path) const {
  auto canonical_path = path::utils::PathToRefValueMap(path);
  const Json::Value from_path = JsonValueFromPath(path);
  return path_to_node_map_.find(canonical_path);
}

} // namespace openapi

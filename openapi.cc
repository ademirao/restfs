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
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "  "; // assume default for comments is None
  const std::string filecontent = Json::writeString(builder, *data);
  return SimpleFileNode(path, data, filecontent.length(), modes);
}

} // namespace path

namespace openapi {

template <typename InputIt>
std::string join(InputIt first, InputIt last, const std::string &separator = "",
                 const std::string &starter = "{",
                 const std::string &concluder = "}") {
  if (first == last) {
    return starter + concluder;
  }

  std::stringstream ss;
  ss << starter << *first << [&first, &last, &separator, &ss]() {
    for (; first != last; ++first) {
      ss << separator << *first;
    }
    return "";
  }() << concluder;

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

#pragma GCC diagnostic ignored "-Wunused-function"
static const Json::Value &FindOrDie(const Json::Value &value,
                                    const std::string &key) {
  const Json::Value *found = Find(value, key);
  CHECK(found != nullptr);
  return *found;
}

static const Json::Value *FindAbsolutePath(const Json::Value &root,
                                           const path::Path &path) {
  const Json::Value *current = &root;
  auto parts_it = path.begin();
  CHECK(parts_it != path.end());
  ++parts_it;
  for (; parts_it != path.end(); ++parts_it) {
    if (!(current->isObject() || current->isArray() || current->isNull())) {
      LOG(INFO) << "Invalid node " << current->isString() << " for path "
                << path << " part " << *parts_it;
      return nullptr;
    }
    const path::Path part = *parts_it;
    const std::string part_str = parts_it->string();
    const char *part_str_end = part_str.c_str() + part.string().length();
    current = current->find(part_str.c_str(), part_str_end);
    if (current == nullptr)
      return nullptr;
  }
  return current;
}

static const Json::Value &FindAbsolutePath(const Json::Value &root,
                                           const path::Path &path,
                                           const Json::Value &default_value) {
  const Json::Value *value = FindAbsolutePath(root, path);
  if (value == nullptr) {
    return default_value;
  }
  return *value;
}

class NodeFactory final {
public:
  NodeFactory(const Json::Value *root) : root_(root) {}
  ~NodeFactory() {}

  const Json::Value &ResolveRef(const Json::Value &value) const {
    const Json::Value *sub = Find(value, "$ref");
    if (sub == nullptr) {
      return value;
    }

    const std::string &ref_value = sub->asString();
    CHECK(ref_value[0] == '#');
    const path::Path ref_value_path(ref_value.substr(1));
    sub = FindAbsolutePath(*root_, ref_value_path);
    CHECK_M(sub != nullptr, "Couldn't find path " + ref_value_path.string());
    return *sub;
  }

  path::Node WriteOperationNode(const path::Path &path,
                                const Json::Value *json) const {
    // const Json::Value &content =
    //     FindAbsolutePath(*json, "/requestBody/content", Json::Value::null);
    // const Json::Value &application_json =
    //     Find(content, "application/json", Json::Value::null);
    // const Json::Value &schema_json =
    //     ResolveRef(Find(application_json, "schema", Json::Value::null));
    return path::SimpleFileNode(path, json, {S_IWRITE});
  }

  path::Node ReadOperationNode(const path::Path &path,
                               const Json::Value *json) const {
    // const Json::Value &content =
    //     FindAbsolutePath(*json, "/responses/200/content", Json::Value::null);
    // const Json::Value &application_json =
    //     Find(content, "application/json", Json::Value::null);
    // const Json::Value &schema_json =
    //     ResolveRef(Find(application_json, "schema", Json::Value::null));
    return path::SimpleFileNode(path, json, {S_IREAD});
  }

  std::vector<std::string>
  FindRequiredQueryParams(const Json::Value *json) const {
    std::vector<std::string> required_query_params;
    const Json::Value &parameters =
        Find(*json, "parameters", Json::Value::null);
    for (const auto &parameter : parameters) {
      const auto &param = ResolveRef(parameter);
      const auto &in = Find(param, "in", Json::Value::null);
      if (in.asString() != "query")
        continue;

      const auto &required_param = Find(param, "required", Json::Value::null);
      bool required =
          (required_param != Json::Value::null) && required_param.asBool();
      if (required) {
        const auto &name = Find(param, "name", Json::Value::null);
        required_query_params.push_back(name.asString());
      }
    }
    std::sort(required_query_params.begin(), required_query_params.end());
    return required_query_params;
  }

  path::Node OperationNode(const rest::constants::OPERATIONS op,
                           const Json::Value *json) const {
    std::vector<std::string> required_query_params =
        FindRequiredQueryParams(json);
    path::Path filename =
        ((required_query_params.empty())
             ? ""
             : join(required_query_params.begin(), required_query_params.end(),
                    ",", "{", "}.")) +
        std::string(rest::constants::OPERATION_NAMES[op]) + ".json";

    switch (op) {
    case rest::constants::HEAD:
    case rest::constants::GET:
      return ReadOperationNode(filename, json);
    case rest::constants::PATCH:
    case rest::constants::DELETE:
    case rest::constants::POST:
    case rest::constants::PUT:
      return WriteOperationNode(filename, json);
    case rest::constants::INVALID:
    case rest::constants::TRACE:
    case rest::constants::OPTIONS:
      break;
    }
    LOG(FATAL) << "Unsupported operation: " << op;
    return path::Node({{}, {}, nullptr, 0}); // Unreachable
  }

  path::Node EntityOperationNode(const path::Path &path,
                                 const Entity *entity) const {
    return path::SimpleFileNode(path, entity, {entity->modes});
  }

private:
  const Json::Value *root_;
}; // namespace openapi

Directory
NewDirectoryFromJsonValue(const std::string &host,
                          std::unique_ptr<const Json::Value> json_data) {
  NodeFactory factory(json_data.get());
  PathToNodeMap path_to_node_map;
  auto insert_it =
      path_to_node_map.emplace(path::Path("/"), path::DirNode("/", nullptr));
  CHECK(insert_it.second);
  path::Node *root = &insert_it.first->second;
  auto insert_node = [root, &path_to_node_map](const path::Path &in_path,
                                               const path::Node &&node) {
    const auto path = path::utils::PathToRefValueMap(in_path);
    auto insert_pair = path_to_node_map.emplace(path, node);
    if (insert_pair.second) {
      auto parent = path_to_node_map.find(path.parent_path());
      CHECK(parent != path_to_node_map.end());
      parent->second.mutable_children()->push_back(&insert_pair.first->second);
    }
    return insert_pair;
  };

  auto insert_rest_operations_metadata = [&insert_node, &factory](
                                             const path::Path &directory_path,
                                             const Json::Value &value) {
    for (const std::string &op_name : value.getMemberNames()) {
      const auto &op_json = value[op_name];
      const auto it = rest::constants::operations_map().find(op_name);
      CHECK(it != rest::constants::operations_map().end());
      const path::Node node = factory.OperationNode(it->second, &op_json);
      const auto node_path = node.path();
      insert_node(directory_path / node_path, std::move(node));

      const path::Path meta_json =
          directory_path /
          (node_path.filename().stem().string() + ".metadata.json");
      auto insert_pair = insert_node(
          meta_json, path::SimpleFileNode(meta_json.filename(),
                                          node.data<Json::Value>(), {S_IREAD}));
      CHECK_M(insert_pair.second, "Path already exists: " + meta_json.string());
    }
  };
  const path::Path root_meta_json("/metadata.json");
  insert_node(root_meta_json, path::SimpleFileNode(root_meta_json.filename(),
                                                   &(*json_data), {S_IREAD}));

  const Json::Value &paths = (*json_data)["paths"];
  for (auto it = paths.begin(), end = paths.end(); it != end; ++it) {
    const path::Path path = path::utils::PathToRefValueMap(it.key().asString());
    LOG(INFO) << "CURRENT" << path;
    const Json::Value &value = *it;

    [&path, &value, &root, &insert_node, &insert_rest_operations_metadata]() {
      const path::Node *parent = root;
      auto current_path = parent->path();
      for (const path::Path &part : path) {
        current_path /= part;
        auto insert_pair =
            insert_node(current_path, path::DirNode(part, nullptr));
        parent = &insert_pair.first->second;
      }
      LOG(INFO) << "INSERT REST OPERA" << current_path;
      insert_rest_operations_metadata(current_path, value);
    }();
  }

  std::vector<Entity> entities;
  // entities.emplace_back(
  //     (Entity){.path = "/user-operations/users/user.entity.json",
  //              .read_path = "/user-operations/users/get.json",
  //              .write_path = "/user-operations/users/post.json",
  //              .delete_path = "/user-operations/users/delete.json",
  //              .modes = S_IREAD | S_IWRITE});

  // for (const Entity &entity : entities) {
  //   insert_node(entity.path,
  //               factory.EntityOperationNode(entity.path.filename(),
  //               &entity));
  // }

  LOG(INFO) << "::::::" << path_to_node_map;
  return Directory(host, std::move(path_to_node_map), std::move(entities),
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
  return val;
}

PathToNodeMap::const_iterator Directory::find(const path::Path &path) const {
  auto canonical_path = path::utils::PathToRefValueMap(path);
  const Json::Value from_path = JsonValueFromPath(path);
  return path_to_node_map_.find(canonical_path);
}

} // namespace openapi

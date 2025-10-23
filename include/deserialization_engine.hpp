#ifndef DESERIALIZATION_ENGINE_H
#define DESERIALIZATION_ENGINE_H

#include "migr.h"
#include "rapidjson/document.h"

using Document = rapidjson::Document;
using Value = rapidjson::Value;

class DeserializationEngine {
public:
  /* reads the MigrNode node from JSON */
  static std::shared_ptr<MIGRNode> read_node(const Value &json) {
    if (!json.IsObject()) {
      return nullptr;
    }

    MIGRNodeType type = static_cast<MIGRNodeType>(json["type"].GetInt());
    std::string content = std::string(json["content"].GetString());
    auto node = std::make_shared<MIGRNode>(type, content);

    if (json.HasMember("metadata")) {
      const auto &meta = json["metadata"];
      for (auto it = meta.MemberBegin(); it != meta.MemberEnd(); ++it) {
        node->metadata_[it->name.GetString()] = it->value.GetString();
      }
    }

    return node;
  }

  /* reads the [id : MigrNode] nodes map from JSON */
  static std::unordered_map<std::string, std::shared_ptr<MIGRNode>>
  read_nodes(const Value &json) {
    std::unordered_map<std::string, std::shared_ptr<MIGRNode>> nodes;

    if (!json.HasMember("nodes")) {
      return nodes;
    }

    const auto &nob = json["nodes"]; // nodes object
    for (auto it = nob.MemberBegin(); it != nob.MemberEnd(); ++it) {
      std::string id = it->name.GetString();
      auto node = read_node(it->value);
      if (node) {
        node->id_ = id;
        nodes[id] = node;
      }
    }
    return nodes;
  }

  /* builds the parent child hieratchy from the JSON file */
  static void build_hieratchy(
      const Value &json,
      std::unordered_map<std::string, std::shared_ptr<MIGRNode>> &nodes) {
    if (!json.HasMember("nodes")) {
      return;
    }

    const auto &nob = json["nodes"];
    for (auto it = nob.MemberBegin(); it != nob.MemberEnd(); ++it) {
      std::string id = it->name.GetString();
      const auto &node_json = it->value;
      auto node = nodes[id];

      // setting parent of node
      if (node_json.HasMember("parent")) {
        std::string parent_id = node_json["parent"].GetString();
        if (nodes.count(parent_id)) {
          node->parent_ = nodes[parent_id];
        }
      }

      // setting children of node
      if (node_json.HasMember("children")) {
        const auto &children = node_json["children"];
        for (auto &child_val : children.GetArray()) {
          std::string child_id = child_val.GetString();
          if (nodes.count(child_id)) {
            node->children_.push_back(nodes[child_id]);
          }
        }
      }
    }
  }

  /* read edges from JSON file */
  template <typename EdgeType>
  static std::vector<EdgeType> read_edges(const Value &json) {
    std::vector<EdgeType> edges;

    if (!json.HasMember("edges")) {
      return edges;
    }

    const auto &edges_arr = json["edges"];
    for (auto &edge_val : edges_arr.GetArray()) {
      EdgeType edge;
      edge.source_id = edge_val["source"].GetString();
      edge.target_id = edge_val["target"].GetString();
      edge.edge_type = static_cast<MIGREdgeType>(edge_val["type"].GetInt());

      if constexpr (requires { edge.relation_label; }) {
        if (edge_val.HasMember("label")) {
          edge.relation_label = edge_val["label"].GetString();
        }
      }
      edges.push_back(edge);
    }
    return edges;
  }

  /* read string-to-string map from JSON file */
  static std::unordered_map<std::string, std::string>
  read_map(const Value &json, const char *key) {
    std::unordered_map<std::string, std::string> map;

    if (!json.HasMember(key)) {
      return map;
    }

    const auto &obj = json[key];
    for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
      map[it->name.GetString()] = it->value.GetString();
    }

    return map;
  }

  /* read string-to-vector map (indexes) from JSON file */
  static std::unordered_map<std::string, std::vector<size_t>>
  read_index(const Value &json, const char *key) {
    std::unordered_map<std::string, std::vector<size_t>> map;

    if (!json.HasMember(key)) {
      return map;
    }

    const auto &obj = json[key];
    for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
      std::string id = it->name.GetString();
      std::vector<size_t> vec;
      for (auto &val : it->value.GetArray()) {
        vec.push_back(val.GetUint64());
      }
      map[id] = vec;
    }
    return map;
  }
};

#endif //! DESERIALIZATION_ENGINE_H

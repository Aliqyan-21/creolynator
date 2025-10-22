#include "serialization_engine.h"

void SerialzationEngine::write_node(Writer &w,
                                    const std::shared_ptr<MIGRNode> &node) {
  if (!node) {
    return;
  }

  w.Key(node->id_.c_str());
  w.StartObject();
  w.Key("type");
  w.Int(static_cast<int>(node->type_));
  w.Key("content");
  w.String(node->content_.c_str());

  if (!node->metadata_.empty()) {
    w.Key("metadata");
    w.StartObject();
    for (const auto &[k, v] : node->metadata_) {
      w.Key(k.c_str());
      w.String(v.c_str());
    }
    w.EndObject();
  }

  w.Key("children");
  w.StartArray();
  for (const auto &child : node->children_) {
    if (child)
      w.String(child->id_.c_str());
  }
  w.EndArray();

  if (auto parent = node->parent_.lock()) {
    w.Key("parent");
    w.String(parent->id_.c_str());
  }

  w.EndObject();
}

void SerialzationEngine::write_nodes(
    Writer &w,
    const std::unordered_map<std::string, std::shared_ptr<MIGRNode>> &nodes) {
  w.Key("nodes");
  w.StartObject();
  for (const auto &[id, node] : nodes) {
    write_node(w, node);
  }
  w.EndObject();
}

template <typename EdgeType>
void SerialzationEngine::write_edge(Writer &w, const EdgeType &edge) {
  w.StartObject();
  w.Key("source");
  w.String(edge.source_id.c_str());
  w.Key("target");
  w.String(edge.target_id.c_str());
  w.Key("type");
  w.Int(static_cast<int>(edge.edge_type));
  if constexpr (requires { edge.relation_label; }) {
    w.Key("label");
    w.String(edge.relation_label.c_str());
  }
  w.EndObject();
}

template <typename EdgeType>
void SerialzationEngine::write_edges(Writer &w,
                                     const std::vector<EdgeType> &edges) {

  w.Key("edges");
  w.StartArray();
  for (const auto &edge : edges) {
    write_edge(w, edge);
  }
  w.EndArray();
}

void SerialzationEngine::write_map(
    Writer &w, const char *key,
    const std::unordered_map<std::string, std::string> &map) {
  w.Key(key);
  w.StartObject();
  for (const auto &[k, v] : map) {
    w.Key(k.c_str());
    w.String(v.c_str());
  }
  w.EndObject();
}

void SerialzationEngine::write_index(
    Writer &w, const char *key,
    const std::unordered_map<std::string, std::vector<size_t>> &idx) {
  w.Key(key);
  w.StartObject();
  for (const auto &[id, vec] : idx) {
    w.Key(id.c_str());
    w.StartArray();
    for (size_t i : vec)
      w.Uint64(i);
    w.EndArray();
  }
  w.EndObject();
}

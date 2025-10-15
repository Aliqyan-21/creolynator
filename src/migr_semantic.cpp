#include "migr_semantic.h"
#include "globals.h"
#include <algorithm>

void SemanticLayer::add_node(std::shared_ptr<MIGRNode> node) {
  if (node) {
    semantic_nodes_[node->id_] = node;
  }
}

void SemanticLayer::remove_node(const std::string &node_id) {
  semantic_nodes_.erase(node_id);
  edges_.erase(node_id);
  backlinks_.erase(node_id);

  for (auto &edge_pair : edges_) {
    auto &edge_list = edge_pair.second;
    edge_list.erase(std::remove(edge_list.begin(), edge_list.end(), node_id),
                    edge_list.end());
  }
}

std::vector<std::shared_ptr<MIGRNode>>
SemanticLayer::query_nodes(std::function<bool(const MIGRNode &)> predicate) {
  std::vector<std::shared_ptr<MIGRNode>> query_results;
  for (const auto &[_, node] : semantic_nodes_) {
    if (node && predicate(*node)) {
      query_results.push_back(node);
    }
  }
  return query_results;
}

void SemanticLayer::serialize(std::ostream &out) const {
  out << "  \"semantic_layer\": {\n";
  out << "    \"edges\": [\n";

  bool first{true};
  for (const auto [source, targets] : edges_) {
    for (const std::string &target : targets) {
      if (!first) {
        out << ",\n";
      }
      first = false;
      out << "      {\"source\": \"" << source << "\", \"target\": \"" << target
          << "\", \"type\": \"SEMANTIC_LINK\"}";
    }
  }
  out << "\n    ]\n";
  out << "  }\n";
}

void StructuralLayer::deserialize(std::istream &in) const {
  // todo: implement deserialization of data - would implement
  // json parsing as data will be in json format
  SPEAK << "Deserialization is not implemented yet" << std::endl;
}

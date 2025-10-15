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
  for (const auto &[source, targets] : edges_) {
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

void SemanticLayer::deserialize(std::istream &in) const {
  // todo: implement deserialization of data - would implement
  // json parsing as data will be in json format
  SPEAK << "Deserialization is not implemented yet" << std::endl;
}

void SemanticLayer::extract_semantics(const StructuralLayer &structural) {
  auto root = structural.get_root();
  if (root) {
    extract_links(root);
    extract_tags(root);
    build_backlinks();
  }
}

void SemanticLayer::add_cross_reference(const std::string &from_id,
                                        const std::string &to_id,
                                        const std::string &relation_type) {
  edges_[from_id].push_back(to_id);
  backlinks_[to_id].push_back(from_id);
  // note: we can use relation type but don't know how?
}

std::vector<std::shared_ptr<MIGRNode>>
SemanticLayer::find_backlinks(const std::string &target_id) {
  std::vector<std::shared_ptr<MIGRNode>> results;
  auto it = backlinks_.find(target_id);
  if (it != backlinks_.end()) {
    for (const std::string &bl_id : it->second) {
      auto node_it = semantic_nodes_.find(bl_id);
      if (node_it != semantic_nodes_.end()) {
        results.push_back(node_it->second);
      }
    }
  }
  return results;
}

void SemanticLayer::extract_links(std::shared_ptr<MIGRNode> node) {
  // todo: implement
  // for now I can think of:
  // - have to look for link patterns in content '[[link]]'.
  // - then if found matches, then create a reference node.
  // - and if node will have children then we can recursively link in them.
}

void SemanticLayer::extract_tags(std::shared_ptr<MIGRNode> node) {
  // todo: implement
  // for now I can think of:
  // - have to look for tag patterns in content '#tag'.
  // - then if found matches, then create a tag node.
  // - and if node will have children then we can recursively link in them.
}

void SemanticLayer::build_backlinks() {
  backlinks_.clear();
  for (const auto &[source, targets] : edges_) {
    for (const auto &target : targets) {
      backlinks_[target].push_back(source);
    }
  }
}

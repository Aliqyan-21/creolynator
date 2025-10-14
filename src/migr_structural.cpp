#include "migr_structural.h"
#include "globals.h"

StructuralLayer::StructuralLayer() {
  root_ = std::make_shared<MIGRNode>(MIGRNodeType::DOCUMENT_ROOT);
  nodes_[root_->id_] = root_;
  parent_stack_.push(root_);
}

void StructuralLayer::add_node(std::shared_ptr<MIGRNode> node) {
  if (node) {
    nodes_[node->id_] = node;
  }
}

void StructuralLayer::remove_node(const std::string &node_id) {
  auto it = nodes_.find(node_id);
  if (it != nodes_.end()) {
    auto node = it->second;
    // safely accessing std::shared_ptr of std::weak_ptr parent
    if (auto parent_ptr = node->parent_.lock()) {
      parent_ptr->remove_child(node_id);
    }
    nodes_.erase(it);
  }
}

std::vector<std::shared_ptr<MIGRNode>>
StructuralLayer::query_nodes(std::function<bool(const MIGRNode &)> predicate) {
  std::vector<std::shared_ptr<MIGRNode>> query_results;
  for (const auto &[_, node] : nodes_) {
    if (node && predicate(*node)) {
      query_results.push_back(node);
    }
  }
  return query_results;
}

void StructuralLayer::serialize(std::ostream &out) const {
  // todo: implement serialization of data
  SPEAK << "Serialization is not implemented yet" << std::endl;
}

void StructuralLayer::deserialize(std::istream &in) const {
  // todo: implement deserialization of data - would implement
  // json parsing as data will be in json format
  SPEAK << "Deserialization is not implemented yet" << std::endl;
}

void StructuralLayer::build_from_tokens(const std::vector<BToken> &tokens) {
  for (size_t i{0}; i < tokens.size(); ++i) {
    const auto &token = tokens[i];

    switch (token.type) {
    case BlockTokenType::HEADING:
      process_heading_token(token);
      break;
    case BlockTokenType::PARAGRAPH:
      process_paragraph_token(token);
      break;
    case BlockTokenType::ULISTITEM:
      process_ulist_token(token);
      break;
    case BlockTokenType::OLISTITEM:
      process_olist_token(token);
      break;
    default:
      break;
    }
  }
}

std::shared_ptr<MIGRNode> StructuralLayer::get_root() const { return root_; }

void StructuralLayer::process_heading_token(const BToken &token) {
  // todo: implement
}
void StructuralLayer::process_paragraph_token(const BToken &token) {
  // todo: implement
}
void StructuralLayer::process_ulist_token(const BToken &token) {
  // todo: implement
}
void StructuralLayer::process_olist_token(const BToken &token) {
  // todo: implement
}

#include "migr_structural.h"
#include "globals.h"
#include <memory>
#include <string>

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
    // todo: later we will handle more block tokens
    default:
      break;
    }
  }
}

std::shared_ptr<MIGRNode> StructuralLayer::get_root() const { return root_; }

void StructuralLayer::process_heading_token(const BToken &token) {
  int level{1}; // default

  if (token.level.has_value()) {
    level = token.level.value();
  }

  manage_heading_stack(level);

  auto heading_node =
      std::make_shared<MIGRNode>(MIGRNodeType::HEADING, token.text.value());
  heading_node->metadata_["level"] = std::to_string(level);

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(heading_node);
  }

  parent_stack_.push(heading_node);
  add_node(heading_node);

  // todo: process inline content then
}
void StructuralLayer::process_paragraph_token(const BToken &token) {
  auto para_node =
      std::make_shared<MIGRNode>(MIGRNodeType::PARAGRAPH, token.text.value());

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(para_node);
  }

  add_node(para_node);

  // todo: process inline content then
}
void StructuralLayer::process_ulist_token(const BToken &token) {
  auto ulist_node =
      std::make_shared<MIGRNode>(MIGRNodeType::ULIST_ITEM, token.text.value());

  if (!list_stack_.empty()) {
    list_stack_.top()->add_child(ulist_node);
  } else {
    auto list_node = std::make_shared<MIGRNode>(MIGRNodeType::ULIST_ITEM);
    if (!parent_stack_.empty()) {
      parent_stack_.top()->add_child(list_node);
    }
    enter_list_context(list_node);
    add_node(list_node);
    list_node->add_child(ulist_node);
  }

  add_node(ulist_node);

  // todo: process inline content then
}
void StructuralLayer::process_olist_token(const BToken &token) {
  auto olist_node =
      std::make_shared<MIGRNode>(MIGRNodeType::OLIST_ITEM, token.text.value());

  if (!list_stack_.empty()) {
    list_stack_.top()->add_child(olist_node);
  } else {
    auto list_node = std::make_shared<MIGRNode>(MIGRNodeType::OLIST_ITEM);
    if (!parent_stack_.empty()) {
      parent_stack_.top()->add_child(list_node);
    }
    enter_list_context(list_node);
    add_node(list_node);
    list_node->add_child(olist_node);
  }

  add_node(olist_node);

  // todo: process inline content then
}

void StructuralLayer::manage_heading_stack(int heading_level) {
  // pop until find appropriate parent level
  while (parent_stack_.size() > 1) {
    auto curr = parent_stack_.top();
    if (curr->type_ == MIGRNodeType::HEADING) {
      auto curr_level_it = curr->metadata_.find("level");
      if (curr_level_it != curr->metadata_.end()) {
        int curr_level = std::stoi(curr_level_it->second);
        if (curr_level < heading_level) {
          break;
        }
      }
    } else if (curr->type_ == MIGRNodeType::DOCUMENT_ROOT) {
      break;
    }
    parent_stack_.pop();
  }
}

void StructuralLayer::enter_list_context(std::shared_ptr<MIGRNode> list_node) {
  list_stack_.push(list_node);
}

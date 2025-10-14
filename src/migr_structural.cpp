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
  // NOTE: a very simple json like serialization
  out << "{\n";
  out << "  \"structural_layer\": {\n";
  out << "    \"root\": \"" << root_->id_ << "\",\n";
  out << "      \"nodes\": {\n";

  bool first = true;
  for (const auto &pair : nodes_) {
    if (!first) {
      out << ",\n";
    }
    first = false;

    auto node = pair.second;
    out << "      \"" << node->id_ << "\": {\n";
    out << "        \"type\": " << static_cast<int>(node->type_) << ",\n";
    out << "        \"content\": \"" << node->content_ << "\",\n";
    out << "        \"children\": [";

    bool first_child = true;
    for (const auto &child : node->children_) {
      if (!first_child) {
        out << ", ";
      }
      first_child = false;
      out << "\"" << child->id_ << "\"";
    }
    out << "]\n";
    out << "      }";
  }
  out << "\n    }\n";
  out << "  }\n";
  out << "}";
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
    case BlockTokenType::HORIZONTALRULE:
      process_horizontal_rule_token(token);
      break;
    case BlockTokenType::VERBATIMBLOCK:
      process_verbatim_token(token);
      break;
    case BlockTokenType::IMAGE:
      process_image_token(token);
      break;
    case BlockTokenType::NEWLINE:
      process_newline_token(token);
      break;
    default:
      // todo: some error handling and recovery mechanism needed
      break;
    }
  }
  // cleaning up any remaining list context
  while (in_list_context()) {
    list_stack_.pop();
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

  process_inline_content(heading_node, token.text.value());
}
void StructuralLayer::process_paragraph_token(const BToken &token) {
  auto para_node =
      std::make_shared<MIGRNode>(MIGRNodeType::PARAGRAPH, token.text.value());

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(para_node);
  }

  add_node(para_node);

  process_inline_content(para_node, token.text.value());
}
void StructuralLayer::process_ulist_token(const BToken &token) {
  auto ulist_node =
      std::make_shared<MIGRNode>(MIGRNodeType::ULIST_ITEM, token.text.value());

  if (in_list_context()) {
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

  process_inline_content(ulist_node, token.text.value());
}
void StructuralLayer::process_olist_token(const BToken &token) {
  auto olist_node =
      std::make_shared<MIGRNode>(MIGRNodeType::OLIST_ITEM, token.text.value());

  if (in_list_context()) {
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

  process_inline_content(olist_node, token.text.value());
}

void StructuralLayer::process_horizontal_rule_token(const BToken &token) {
  auto hr_node = std::make_shared<MIGRNode>(MIGRNodeType::HORIZONTAL_RULE);

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(hr_node);
  }

  add_node(hr_node);
}

void StructuralLayer::process_verbatim_token(const BToken &token) {
  auto verb_node = std::make_shared<MIGRNode>(MIGRNodeType::VERBATIM_BLOCK,
                                              token.text.value_or(""));

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(verb_node);
  }

  add_node(verb_node);
}

void StructuralLayer::process_image_token(const BToken &token) {
  auto image_node =
      std::make_shared<MIGRNode>(MIGRNodeType::IMAGE, token.text.value_or(""));

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(image_node);
  }

  add_node(image_node);
}

void StructuralLayer::process_newline_token(const BToken &token) {
  auto newline_node = std::make_shared<MIGRNode>(MIGRNodeType::NEWLINE);

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(newline_node);
  }

  add_node(newline_node);
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

void StructuralLayer::enter_list_context(std::shared_ptr<MIGRNode> list_type) {
  auto list_node = std::make_shared<MIGRNode>(list_type);
  list_stack_.push(list_node);

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(list_node);
  }

  add_node(list_node);
  list_stack_.push(list_node);
}

void StructuralLayer::exit_list_context() {
  if (in_list_context()) {
    list_stack_.pop();
  }
}

bool StructuralLayer::in_list_context() const { return !list_stack_.empty(); }

void StructuralLayer::process_inline_content(std::shared_ptr<MIGRNode> parent,
                                             const std::string &content) {
  if (content.empty()) {
    return;
  }

  // NOTE: this is just for now testing block types
  // TODO: we will use inline tokens (ILexer) originally ofc
  auto text_node = std::make_shared<MIGRNode>(MIGRNodeType::TEXT, content);
  parent->add_child(text_node);
  add_node(text_node);
}

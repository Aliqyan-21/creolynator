#include "migr_structural.h"
#include "globals.h"
#include <error.h>
#include <memory>
#include <string>

StructuralLayer::StructuralLayer()
    : recovery_strategy_(RecoveryStrategy::ATTACH_TO_PARENT) {
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
  clear_errors();

  for (size_t i{0}; i < tokens.size(); ++i) {
    const auto &token = tokens[i];

    // break out of list context for non list items
    if (in_list_context() && token.type != BlockTokenType::ULISTITEM &&
        token.type != BlockTokenType::OLISTITEM) {
      while (in_list_context()) {
        exit_list_context();
      }
    }

    try {
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
        handle_error("Unknown block token type", i);
        if (!attempt_recovery(token)) {
          throw MIGRError("Failed to recover from unkown token", i, "skip");
        }
        break;
      }
    } catch (const MIGRError &e) {
      errors_.push_back(e);
      if (e.get_severity() == MIGRError::Severity::FATAL) {
        throw;
      }
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

  level = token.level.value_or(1);

  manage_heading_stack(level);

  auto heading_node = std::make_shared<MIGRNode>(MIGRNodeType::HEADING,
                                                 token.text.value_or(""));
  heading_node->metadata_["level"] = std::to_string(level);
  heading_node->loc_ = token.loc;

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(heading_node);
  }

  parent_stack_.push(heading_node);
  add_node(heading_node);

  process_inline_content(heading_node, token.text.value_or(""));
}

void StructuralLayer::process_paragraph_token(const BToken &token) {
  auto para_node = std::make_shared<MIGRNode>(MIGRNodeType::PARAGRAPH,
                                              token.text.value_or(""));

  para_node->loc_ = token.loc;

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(para_node);
  }

  add_node(para_node);

  process_inline_content(para_node, token.text.value_or(""));
}

void StructuralLayer::process_ulist_token(const BToken &token) {
  int level = token.level.value_or(1);

  while ((int)list_stack_.size() > level) {
    exit_list_context();
  }

  if ((int)list_stack_.size() < level || !in_list_context()) {
    enter_list_context(MIGRNodeType::ULIST);
  }

  auto list_item_node = std::make_shared<MIGRNode>(MIGRNodeType::ULIST_ITEM,
                                                   token.text.value_or(""));
  list_item_node->loc_ = token.loc;

  if (in_list_context()) {
    list_stack_.top()->add_child(list_item_node);
  } else {
    if (!parent_stack_.empty()) {
      parent_stack_.top()->add_child(list_item_node);
    }
  }

  add_node(list_item_node);
  process_inline_content(list_item_node, token.text.value_or(""));
}

void StructuralLayer::process_olist_token(const BToken &token) {
  int level = token.level.value_or(1);

  while ((int)list_stack_.size() > level) {
    exit_list_context();
  }

  if ((int)list_stack_.size() < level || !in_list_context()) {
    enter_list_context(MIGRNodeType::OLIST);
  }

  auto list_item_node = std::make_shared<MIGRNode>(MIGRNodeType::OLIST_ITEM,
                                                   token.text.value_or(""));
  list_item_node->loc_ = token.loc;

  if (in_list_context()) {
    list_stack_.top()->add_child(list_item_node);
  } else {
    if (!parent_stack_.empty()) {
      parent_stack_.top()->add_child(list_item_node);
    }
  }

  add_node(list_item_node);
  process_inline_content(list_item_node, token.text.value_or(""));
}

void StructuralLayer::process_horizontal_rule_token(const BToken &token) {
  auto hr_node = std::make_shared<MIGRNode>(MIGRNodeType::HORIZONTAL_RULE);
  hr_node->loc_ = token.loc;

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(hr_node);
  }

  add_node(hr_node);
}

void StructuralLayer::process_verbatim_token(const BToken &token) {
  auto verb_node = std::make_shared<MIGRNode>(MIGRNodeType::VERBATIM_BLOCK,
                                              token.text.value_or(""));
  verb_node->loc_ = token.loc;

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(verb_node);
  }

  add_node(verb_node);
}

void StructuralLayer::process_image_token(const BToken &token) {
  auto image_node =
      std::make_shared<MIGRNode>(MIGRNodeType::IMAGE, token.text.value_or(""));
  image_node->loc_ = token.loc;

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(image_node);
  }

  add_node(image_node);
}

void StructuralLayer::process_newline_token(const BToken &token) {
  auto newline_node = std::make_shared<MIGRNode>(MIGRNodeType::NEWLINE);
  newline_node->loc_ = token.loc;

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

void StructuralLayer::enter_list_context(MIGRNodeType list_type) {
  auto list_node = std::make_shared<MIGRNode>(list_type);

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
  ILexer i_lexer;
  auto i_tokens = i_lexer.tokenize(content, parent->loc_);

  for (const auto &i_token : i_tokens) {
    auto inline_node = convert_i_tokens_to_migr_node(i_token);
    if (inline_node) {
      parent->add_child(inline_node);
      add_node(inline_node);
    }
  }
}

std::shared_ptr<MIGRNode>
StructuralLayer::convert_i_tokens_to_migr_node(const IToken &i_token) {
  /* TODO: it's just like processing the block tokens, and converting them to
   * nodes, but this time we doing it in one function.
   * - we will just have to handle different inline token types.
   * - then according to the types, we will make nodes, with content
   * - if node will have url, we will add it in metadata
   * - for nested formatting we will have to process children too -> so we will
   * make the function recursive ofc. */
  SPEAK << "convert_i_tokens_to_migr_node not implemented yet, and was called"
        << std::endl;
  return nullptr;
}

//------------------------------------------//
//      Error Handling and Recovery         //
//------------------------------------------//

void StructuralLayer::set_recovery_stratgegy(RecoveryStrategy strategy) {
  recovery_strategy_ = strategy;
}

const std::vector<MIGRError> &StructuralLayer::ger_errors() { return errors_; }

void StructuralLayer::clear_errors() { errors_.clear(); }

void StructuralLayer::handle_error(const std::string &message, size_t line) {
  errors_.emplace_back(message, line, "attempting recovery");
}

bool StructuralLayer::attempt_recovery(const BToken &token) {
  switch (recovery_strategy_) {
  case RecoveryStrategy::SKIP:
    // just skip
    return true;
  case RecoveryStrategy::ATTACH_TO_PARENT:
    // just creating generic node and attaching to parent
    if (!parent_stack_.empty()) {
      auto recovery_node = std::make_shared<MIGRNode>(MIGRNodeType::PARAGRAPH,
                                                      token.text.value_or(""));
      parent_stack_.top()->add_child(recovery_node);
      add_node(recovery_node);
      return true;
    }
    return false;
  case RecoveryStrategy::CREATE_PLACEHOLDER:
    // creating placeholder node
    auto placeholder = std::make_shared<MIGRNode>(
        MIGRNodeType::PARAGRAPH,
        "[PLACEHOLDER: " + token.text.value_or("") + "]");
    if (!parent_stack_.empty()) {
      parent_stack_.top()->add_child(placeholder);
    }
    add_node(placeholder);
    return true;
  }
  return false;
}

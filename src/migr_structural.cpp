#include "migr_structural.h"
#include "globals.h"
#include "serialization_engine.hpp"
#include <algorithm>
#include <error.h>
#include <memory>
#include <string>

StructuralLayer::StructuralLayer()
    : recovery_strategy_(RecoveryStrategy::ATTACH_TO_PARENT) {
  root_ = std::make_shared<MIGRNode>(MIGRNodeType::DOCUMENT_ROOT);
  nodes_[root_->id_] = root_;
  parent_stack_.push(root_);
}

//--------------------------//
//   MIGR Graph Interface   //
//--------------------------//

/*
 * Adds a node to the nodes_ map.
 * Inserts or overwrites based on node's unique id.
 * Does nothing if the node pointer is null.
 */
void StructuralLayer::add_node(std::shared_ptr<MIGRNode> node) {
  if (node) {
    nodes_[node->id_] = node;
  }
}

/*
 * Removes a node by id from nodes_ map.
 * If node exists, removes it from its parent’s children list if any.
 * Then erases node from the internal nodes map.
 */
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

/*
 * Takes a predicate (true/false) function as input
 * Queries nodes by using that function.
 * Returns a vector of shared_ptrs containing nodes matching the predicate.
 */
std::vector<std::shared_ptr<MIGRNode>> StructuralLayer::query_nodes(
    std::function<bool(const MIGRNode &)> predicate) const {
  std::vector<std::shared_ptr<MIGRNode>> query_results;
  for (const auto &[_, node] : nodes_) {
    if (node && predicate(*node)) {
      query_results.push_back(node);
    }
  }
  return query_results;
}

/*
 * Serializes the StructuralLayer into a strong JSON format.
 * Outputs root id, each node’s type, content, metadata, and child
 * relationships.
 */
void StructuralLayer::serialize(std::ostream &out) const {
  rapidjson::StringBuffer buffer;
  Writer writer(buffer);

  writer.StartObject();
  writer.Key("structural_layer");
  writer.StartObject();

  writer.Key("version");
  writer.String("1.0");
  writer.Key("root");
  writer.String(root_ ? root_->id_.c_str() : "");

  SerialzationEngine::write_nodes(writer, nodes_);

  writer.EndObject();
  writer.EndObject();

  out << buffer.GetString();
}

/*
 * Deserializes the json data, into structural layer nodes
 */
void StructuralLayer::deserialize(std::istream &in) {
}

/*
 * Constructs structural layer from a sequence of Block Tokens from our BLexer
 * Processes tokens according to block types (headings, paragraphs, lists).
 * Handles list context transitions and error recovery per configured strategy.
 * Collects errors encountered during building
 */
void StructuralLayer::build_from_tokens(const std::vector<BToken> &tokens) {
  clear_errors();

  _V_ << " [StructuralLayer] Building Structural Layer From Tokens..."
      << std::endl;
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
  _V_ << " [StructuralLayer] Structural Layer Built." << std::endl;
}

/*
 * Returns root node of the StructuralLayer.
 */
std::shared_ptr<MIGRNode> StructuralLayer::get_root() const { return root_; }

//-----------------------//
//      Processors       //
//-----------------------//

/*
 * Processes a heading token:
 * Determines heading level, manages heading stack, creates a heading node with
 * metadata. Adds it as a child to the current parent in the stack, and pushes
 * it to the parent stack. And then processes inline content for the created
 * node.
 */
void StructuralLayer::process_heading_token(const BToken &token) {
  _V_ << " [StructuralLayer] Creating Heading Node." << std::endl;
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

/*
 * Processes a paragraph token:
 * Creates a paragraph node, adds it as child to current top parent.
 * And then processes inline content for the created node.
 */
void StructuralLayer::process_paragraph_token(const BToken &token) {
  _V_ << " [StructuralLayer] Creating Paragraph Node." << std::endl;
  auto para_node = std::make_shared<MIGRNode>(MIGRNodeType::PARAGRAPH,
                                              token.text.value_or(""));

  para_node->loc_ = token.loc;

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(para_node);
  }

  add_node(para_node);

  process_inline_content(para_node, token.text.value_or(""));
}

/*
 * Processes an unordered list token:
 * Manages the list context stack depth according to list level.
 * Creates a ULIST_ITEM node, adds it as child.
 * and then process inline tokens.
 */
void StructuralLayer::process_ulist_token(const BToken &token) {
  _V_ << " [StructuralLayer] Creating Unordered List Node." << std::endl;
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

/*
 * Processes an ordered list (OLIST) token:
 * Similar handling as unordered lists but for OLIST and OLIST_ITEM types.
 */
void StructuralLayer::process_olist_token(const BToken &token) {
  _V_ << " [StructuralLayer] Creating Ordered List Node." << std::endl;
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

/*
 * Processes a horizontal rule token:
 * Creates a horizontal rule node and attaches it to current parent.
 */
void StructuralLayer::process_horizontal_rule_token(const BToken &token) {
  _V_ << " [StructuralLayer] Creating Horizontal Rule Node." << std::endl;
  auto hr_node = std::make_shared<MIGRNode>(MIGRNodeType::HORIZONTAL_RULE);
  hr_node->loc_ = token.loc;

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(hr_node);
  }

  add_node(hr_node);
}

/*
 * Processes a verbatim block token:
 * Creates a verbatim block node, attaches to parent, and adds to map.
 */
void StructuralLayer::process_verbatim_token(const BToken &token) {
  _V_ << " [StructuralLayer] Creating Verbatim Node." << std::endl;
  auto verb_node = std::make_shared<MIGRNode>(MIGRNodeType::VERBATIM_BLOCK,
                                              token.text.value_or(""));
  verb_node->loc_ = token.loc;

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(verb_node);
  }

  add_node(verb_node);
}

/*
 * Processes an image token:
 * Creates an image node with associated text, attaches to current parent
 * adds to map
 */
void StructuralLayer::process_image_token(const BToken &token) {
  _V_ << " [StructuralLayer] Creating Image Node." << std::endl;
  auto image_node =
      std::make_shared<MIGRNode>(MIGRNodeType::IMAGE, token.text.value_or(""));
  image_node->loc_ = token.loc;

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(image_node);
  }

  add_node(image_node);
}

/*
 * Processes a newline token:
 * Creates a newline node, attaches, and adds to map.
 */
void StructuralLayer::process_newline_token(const BToken &token) {
  _V_ << " [StructuralLayer] Creating Newline Node." << std::endl;
  auto newline_node = std::make_shared<MIGRNode>(MIGRNodeType::NEWLINE);
  newline_node->loc_ = token.loc;

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(newline_node);
  }

  add_node(newline_node);
}

//---------------------------//
//      Stack Management     //
//---------------------------//

/*
 * Manages the heading stack by popping until appropriate parent for the heading
 * level is found. Ensures proper nesting of headings in the structural tree.
 */
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

/*
 * Creates a list node, attaches it
 * and pushes onto the list stack, and node map
 */
void StructuralLayer::enter_list_context(MIGRNodeType list_type) {
  auto list_node = std::make_shared<MIGRNode>(list_type);

  if (!parent_stack_.empty()) {
    parent_stack_.top()->add_child(list_node);
  }

  add_node(list_node);
  list_stack_.push(list_node);
}

/*
 * Exits the current list context by popping from the list stack.
 * Does nothing if not already in list context
 */
void StructuralLayer::exit_list_context() {
  if (in_list_context()) {
    list_stack_.pop();
  }
}

/*
 * Checks if currently inside a list context by
 * simply returning true if list stack is not empty.
 */
bool StructuralLayer::in_list_context() const { return !list_stack_.empty(); }

//--------------------------//
//     Inline Processing    //
//--------------------------//

/*
 * Processes inline tokens for a given blockk token MIGR node node from provided
 * text content. Uses our ILexer to generate inline tokens, converts them to
 * MIGRNodes recursively, adds inline nodes as children under the parent, and to
 * the node map.
 */
void StructuralLayer::process_inline_content(std::shared_ptr<MIGRNode> parent,
                                             const std::string &content) {
  _V_ << " [StructuralLayer] Processing Inline Tokens for parent id: "
      << parent->id_ << "..." << std::endl;
  if (content.empty()) {
    return;
  }
  /* NOTE: we can use BLexer::process_inline_tokens but I am letting it be a
   * isolated feature of our lexer and here we will be independently
   * construting inline tokens */
  ILexer i_lexer;
  auto i_tokens = i_lexer.tokenize(content, parent->loc_);

  for (const auto &i_token : i_tokens) {
    auto inline_node = convert_i_tokens_to_migr_node(i_token);
    if (inline_node) {
      parent->add_child(inline_node);
      add_node(inline_node);
    }
  }

  _V_ << " [StructuralLayer] Inline Tokens Processed for parent id: "
      << parent->id_ << "." << std::endl;
}

/*
 * Converts an IToken (inline token) into a MIGRNode.
 * Maps inline token types to MIGRNodeTypes.
 * Also handles URLs for links and images.
 * It's a recursive function to convert and add children tokens
 * (except for links/images)
 */
std::shared_ptr<MIGRNode>
StructuralLayer::convert_i_tokens_to_migr_node(const IToken &i_token) {
  _V_ << "Converting InlineTokenTypes to MigrNodeTypes..." << std::endl;
  MIGRNodeType nt;
  std::string content = i_token.content.value_or("");
  std::string url = i_token.url.value_or("");

  switch (i_token.type) {
  case InlineTokenType::TEXT:
    nt = MIGRNodeType::TEXT;
    break;
  case InlineTokenType::BOLD:
    nt = MIGRNodeType::BOLD;
    break;
  case InlineTokenType::ITALIC:
    nt = MIGRNodeType::ITALIC;
    break;
  case InlineTokenType::LINK:
    nt = MIGRNodeType::LINK;
    break;
  case InlineTokenType::IMAGE:
    nt = MIGRNodeType::IMAGE;
    break;
  case InlineTokenType::VERBATIM:
    nt = MIGRNodeType::VERBATIM_INLINE;
    break;
  case InlineTokenType::LINEBREAK:
    nt = MIGRNodeType::LINEBREAK;
    break;
  case InlineTokenType::ESCAPE:
    // note: escape chars we treat as text
    nt = MIGRNodeType::TEXT;
    break;
  default:
    nt = MIGRNodeType::TEXT;
    break;
  }

  auto node = std::make_shared<MIGRNode>(nt, content);

  if (!url.empty() && (nt == MIGRNodeType::LINK || nt == MIGRNodeType::IMAGE)) {
    node->metadata_["url"] = url;
  }

  /* for nested formatting we will recursively run the function */
  if (i_token.type != InlineTokenType::LINK &&
      i_token.type != InlineTokenType::IMAGE) {
    for (const auto &child_token : i_token.children) {
      auto child_node = convert_i_tokens_to_migr_node(child_token);
      if (child_node) {
        node->add_child(child_node);
        add_node(child_node);
      }
    }
  }

  _V_ << "Converted InlineTokenTypes to MigrNodeTypes." << std::endl;

  return node;
}

//------------------------------------------//
//      Error Handling and Recovery         //
//------------------------------------------//

/*
 * Public function to set the error recovery strategy for this StructuralLayer
 * object to control behavior when parsing errors occur (skip, attach to
 * parent, create placeholder).
 */
void StructuralLayer::set_recovery_stratgegy(RecoveryStrategy strategy) {
  recovery_strategy_ = strategy;
}

/*
 * Returns a const reference to the vector of errors encountered during
 * building strcutural layer from BTokens
 */
const std::vector<MIGRError> &StructuralLayer::get_errors() { return errors_; }

/*
 * Clears all recorded errors.
 */
void StructuralLayer::clear_errors() { errors_.clear(); }

/*
 * Handles a parsing or structural error by recording it with message and line.
 */
void StructuralLayer::handle_error(const std::string &message, size_t line) {
  errors_.emplace_back(message, line, "attempting recovery");
}

/*
 * Attempts error recovery based on current recovery strategy.
 * - SKIP: silently skips erroneous token.
 * - ATTACH_TO_PARENT: creates a generic 'paragraph' node and attaches to
 * current parent.
 * - CREATE_PLACEHOLDER: creates a 'paragraph' node with placeholder text and
 * attaches. Returns true if recovery was successful | otherwise false.
 */
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

//---------------------//
//    For debugging    //
//---------------------//

void StructuralLayer::print_structural_info(bool detailed) const {
  std::cout << "=== structural info ===" << std::endl;
  std::cout << "Total Nodes: " << nodes_.size() << std::endl;
  std::cout << "Root ID: " << (root_ ? root_->id_ : "[no root]") << std::endl;

  // freq of types of nodes
  std::unordered_map<MIGRNodeType, size_t> counts;
  for (const auto &[_, node] : nodes_) {
    if (node) {
      counts[node->type_]++;
    }
  }

  static const std::unordered_map<MIGRNodeType, std::string> type_names = {
      {MIGRNodeType::DOCUMENT_ROOT, "DOCUMENT_ROOT"},
      {MIGRNodeType::HEADING, "HEADING"},
      {MIGRNodeType::PARAGRAPH, "PARAGRAPH"},
      {MIGRNodeType::ULIST, "ULIST"},
      {MIGRNodeType::ULIST_ITEM, "ULIST_ITEM"},
      {MIGRNodeType::OLIST, "OLIST"},
      {MIGRNodeType::OLIST_ITEM, "OLIST_ITEM"},
      {MIGRNodeType::LINK, "LINK"},
      {MIGRNodeType::IMAGE, "IMAGE"},
      {MIGRNodeType::BOLD, "BOLD"},
      {MIGRNodeType::ITALIC, "ITALIC"},
      {MIGRNodeType::TEXT, "TEXT"},
      {MIGRNodeType::VERBATIM_BLOCK, "VERBATIM_BLOCK"},
      {MIGRNodeType::VERBATIM_INLINE, "VERBATIM_INLINE"},
      {MIGRNodeType::HORIZONTAL_RULE, "HORIZONTAL_RULE"},
      {MIGRNodeType::LINEBREAK, "LINEBREAK"},
      {MIGRNodeType::NEWLINE, "NEWLINE"}};

  std::cout << "\nNode Type Distribution:" << std::endl;
  for (const auto &[type, name] : type_names) {
    if (counts[type] > 0) {
      std::cout << "  " << name << ": " << counts[type] << std::endl;
    }
  }

  if (!detailed || !root_)
    return;

  std::cout << "\n=== more detailed ===\n--- Document Tree Structure ---"
            << std::endl;

  std::function<void(const std::shared_ptr<MIGRNode> &, int)> print_tree =
      [&](const std::shared_ptr<MIGRNode> &node, int depth) {
        if (!node)
          return;

        auto type_it = type_names.find(node->type_);
        std::string type_name =
            type_it != type_names.end() ? type_it->second : "UNKNOWN";

        std::cout << std::string(depth * 2, ' ') << "└─ " << type_name << " ["
                  << node->id_ << "]";

        // metadata
        if (!node->metadata_.empty()) {
          std::cout << " {";
          bool first = true;
          for (const auto &[k, v] : node->metadata_) {
            std::cout << (first ? "" : ", ") << k << ": \"" << v << "\"";
            first = false;
          }
          std::cout << "}";
        }

        // content (truncated)
        if (!node->content_.empty()) {
          std::string content = node->content_;
          if (content.length() > 50)
            content = content.substr(0, 47) + "...";
          std::replace(content.begin(), content.end(), '\n', ' ');
          std::cout << " \"" << content << "\"";
        }

        std::cout << " [children: " << node->children_.size() << "]\n";

        for (const auto &child : node->children_) {
          print_tree(child, depth + 1);
        }
      };

  print_tree(root_, 0);
  std::cout << std::endl;
}

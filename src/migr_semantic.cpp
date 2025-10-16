#include "migr_semantic.h"
#include "globals.h"
#include <algorithm>
#include <memory>

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

std::vector<std::shared_ptr<MIGRNode>> SemanticLayer::query_nodes(
    std::function<bool(const MIGRNode &)> predicate) const {
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
  _V_ << "Extracting semantic info..." << std::endl;

  auto root = structural.get_root();
  if (!root) {
    _V_ << "No root node found while extracting semantics!" << std::endl;
    return;
  }

  auto link_nodes = structural.query_nodes(
      [](const MIGRNode &node) { return node.type_ == MIGRNodeType::LINK; });

  for (auto &node : link_nodes) {
    add_node(node);
  }

  extract_links(root);
  extract_tags(root);
  build_backlinks();
}

void SemanticLayer::add_semantic_edge(const std::string &from_id,
                                      const std::string &to_id,
                                      const std::string &relation_type) {
  edges_[from_id].push_back(to_id);

  /* we can store relation type in metadata */
  auto from_it = semantic_nodes_.find(from_id);
  if (from_it != semantic_nodes_.end()) {
    std::string edge_key = "relation_to_" + to_id;
    from_it->second->metadata_[edge_key] = relation_type;
  }
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

std::vector<std::shared_ptr<MIGRNode>>
SemanticLayer::search_tag(const std::string &tag) {
  std::vector<std::shared_ptr<MIGRNode>> results;

  auto tag_nodes = query_nodes([&](const MIGRNode &node) {
    return node.type_ == MIGRNodeType::TAG && node.content_ == tag;
  });

  for (auto tgn : tag_nodes) {
    results.push_back(tgn);
    auto tgbs = find_backlinks(tgn->id_);
    results.insert(results.end(), tgbs.begin(), tgbs.end());
  }
  return results;
}

std::vector<std::shared_ptr<MIGRNode>>
SemanticLayer::find_all_links_to_target(const std::string &target_name) {
  std::vector<std::shared_ptr<MIGRNode>> results;

  auto ref_nodes = query_nodes([&](const MIGRNode &node) {
    auto it = node.metadata_.find("target");
    return node.type_ == MIGRNodeType::REFERENCE &&
           it != node.metadata_.end() && it->second == target_name;
  });

  for (auto &ref : ref_nodes) {
    auto bls = find_backlinks(ref->id_);
    results.insert(results.end(), bls.begin(), bls.end());
  }
  return results;
}

void SemanticLayer::extract_links(std::shared_ptr<MIGRNode> node) {
  if (!node) {
    return;
  }

  if (node->type_ == MIGRNodeType::LINK) {
    auto url_it = node->metadata_.find("url");
    if (url_it != node->metadata_.end()) {
      std::string target_url = url_it->second;

      // skipping tag linke [[#tag]]
      if (!target_url.empty() && target_url[0] == '#') {
        return;
      }

      std::string ref_id = get_or_create_reference_node(target_url);

      add_semantic_edge(node->id_, ref_id, "references");
    }
  }

  // recursively processing the children
  for (const auto &child : node->children_) {
    extract_links(child);
  }
}

void SemanticLayer::extract_tags(std::shared_ptr<MIGRNode> node) {
  if (!node)
    return;

  // let's do this: tag will be like this [[#tag]] Creole-style tag links
  if (node->type_ == MIGRNodeType::LINK) {
    auto url_it = node->metadata_.find("url");
    if (url_it != node->metadata_.end()) {
      std::string url = url_it->second;

      // confirming that it is a tag link
      if (!url.empty() && url[0] == '#') {
        std::string tag_name = url.substr(1);

        std::string tag_id = get_or_create_tag_node(tag_name);

        add_semantic_edge(node->id_, tag_id, "tagged_with");
      }
    }
  }

  // recursively processing the children
  for (const auto &child : node->children_) {
    extract_tags(child);
  }
}

void SemanticLayer::build_backlinks() {
  backlinks_.clear();
  for (const auto &[source, targets] : edges_) {
    for (const auto &target : targets) {
      backlinks_[target].push_back(source);
    }
  }
}

/* will prevent adding duplicate reference nodes, if node present just get it's
 * id, if not present then create and get it's id */
std::string
SemanticLayer::get_or_create_reference_node(const std::string &target) {
  // checking if reference node already exist for this target
  for (const auto &[id, node] : semantic_nodes_) {
    if (node->type_ == MIGRNodeType::REFERENCE) {
      auto it = node->metadata_.find("target");
      if (it != node->metadata_.end() && it->second == target) {
        return id;
      }
    }
  }

  // creating ref node
  auto ref_node = std::make_shared<MIGRNode>(MIGRNodeType::REFERENCE, target);
  ref_node->metadata_["target"] = target;

  if (target.find("http://") == 0 || target.find("https://") == 0) {
    ref_node->metadata_["link_type"] = "external";
  } else {
    ref_node->metadata_["link_type"] = "internal";
  }

  add_node(ref_node);
  return ref_node->id_;
}

/* will prevent adding duplicate tag nodes, if node present just get it's id, if
 * not present then create and get it's id */
std::string SemanticLayer::get_or_create_tag_node(const std::string &tag_name) {
  // checking if already exists
  for (const auto &[id, node] : semantic_nodes_) {
    if (node->type_ == MIGRNodeType::TAG && node->content_ == tag_name) {
      return id;
    }
  }

  // creating new
  auto tag_node = std::make_shared<MIGRNode>(MIGRNodeType::TAG, tag_name);
  tag_node->metadata_["tag_name"] = tag_name;
  add_node(tag_node);
  return tag_node->id_;
}

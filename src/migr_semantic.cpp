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
  auto it = semantic_nodes_.find(node_id);
  if (it == semantic_nodes_.end()) {
    return;
  }

  auto node = it->second;

  // now remove all edges involving this node.
  edges_.erase(std::remove_if(edges_.begin(), edges_.end(),
                              [&node_id](const SemanticEdge &e) {
                                return e.source_id == node_id ||
                                       e.target_id == node_id;
                              }),
               edges_.end());

  // cleaning up semantic links in other nodes
  for (auto &[id, n] : semantic_nodes_) {
    if (n) {
      n->semantic_links_.erase(
          std::remove_if(n->semantic_links_.begin(), n->semantic_links_.end(),
                         [&node](const std::shared_ptr<MIGRNode> &link) {
                           return link && link->id_ == node->id_;
                         }),
          n->semantic_links_.end());
    }
  }

  // cache clean
  for (auto it{reference_cache_.begin()}; it != reference_cache_.end();) {
    if (it->second == node_id) {
      it = reference_cache_.erase(it);
    } else {
      it++;
    }
  }

  // removing from storage
  semantic_nodes_.erase(it);

  build_backlink_index();
}

std::vector<std::shared_ptr<MIGRNode>> SemanticLayer::query_nodes(
    std::function<bool(const MIGRNode &)> predicate) const {
  std::vector<std::shared_ptr<MIGRNode>> query_results;
  query_results.reserve(semantic_nodes_.size() / 2); // educated guess

  for (const auto &[_, node] : semantic_nodes_) {
    if (node && predicate(*node)) {
      query_results.push_back(node);
    }
  }

  return query_results;
}

void SemanticLayer::serialize(std::ostream &out) const {
  out << "  \"semantic_layer\": {\n";

  // nodes
  out << "    \"semantic_nodes\": {\n";
  bool first_node = true;
  for (const auto &[id, node] : semantic_nodes_) {
    if (!first_node)
      out << ",\n";
    first_node = false;

    out << "      \"" << id << "\": {\n";
    out << "        \"type\": " << static_cast<int>(node->type_) << ",\n";
    out << "        \"content\": \"" << node->content_ << "\",\n";
    out << "        \"metadata\": {";

    bool first_meta = true;
    for (const auto &[key, value] : node->metadata_) {
      if (!first_meta)
        out << ", ";
      first_meta = false;
      out << "\"" << key << "\": \"" << value << "\"";
    }
    out << "}\n";
    out << "      }";
  }
  out << "\n    },\n";

  // and edges
  out << "    \"edges\": [\n";
  bool first_edge = true;
  for (const auto &edge : edges_) {
    if (!first_edge)
      out << ",\n";
    first_edge = false;

    out << "      {\"source\": \"" << edge.source_id << "\", \"target\": \""
        << edge.target_id << "\", \"type\": \"" << edge.relation_label
        << "\", \"edge_type\": " << static_cast<int>(edge.edge_type) << "}";
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

  reset();

  auto link_nodes = structural.query_nodes(
      [](const MIGRNode &node) { return node.type_ == MIGRNodeType::LINK; });

  for (auto &node : link_nodes) {
    add_node(node);
  }

  extract_links(root);
  extract_tags(root);
  build_backlink_index();

  _V_ << " [SemanticLayer] Extraction complete. Total nodes: "
      << semantic_nodes_.size() << ", Edges: " << edges_.size() << std::endl;
}

void SemanticLayer::add_semantic_edge(const std::shared_ptr<MIGRNode> &source,
                                      const std::shared_ptr<MIGRNode> &target,
                                      MIGREdgeType edge_type,
                                      const std::string &relation_label) {
  source->add_semantic_link(target);

  edges_.push_back({source->id_, target->id_, edge_type, relation_label});
}

std::vector<std::shared_ptr<MIGRNode>>
SemanticLayer::find_backlinks(const std::string &target_id) const {
  std::vector<std::shared_ptr<MIGRNode>> results;

  auto it = backlink_index_.find(target_id);
  if (it != backlink_index_.end()) {
    results.reserve(it->second.size());
    for (const std::string &src_id : it->second) {
      auto node_it = semantic_nodes_.find(src_id);
      if (node_it != semantic_nodes_.end()) {
        results.push_back(node_it->second);
      }
    }
  }
  return results;
}

std::vector<std::shared_ptr<MIGRNode>>
SemanticLayer::search_tag(const std::string &tag) const {
  std::vector<std::shared_ptr<MIGRNode>> results;

  auto tag_nodes = query_nodes([&](const MIGRNode &node) {
    return node.type_ == MIGRNodeType::TAG && node.content_ == tag;
  });

  for (const auto &tgn : tag_nodes) {
    results.push_back(tgn);
    auto tgbs = find_backlinks(tgn->id_);
    results.insert(results.end(), tgbs.begin(), tgbs.end());
  }
  return results;
}

std::vector<std::shared_ptr<MIGRNode>>
SemanticLayer::find_all_links_to_target(const std::string &target_name) const {
  std::vector<std::shared_ptr<MIGRNode>> results;

  auto ref_nodes = query_nodes([&](const MIGRNode &n) {
    if (n.type_ != MIGRNodeType::REFERENCE) {
      return false;
    }
    auto it = n.metadata_.find("target");
    return it != n.metadata_.end() && it->second == target_name;
  });

  for (const auto &ref : ref_nodes) {
    auto bls = find_backlinks(ref->id_);
    results.insert(results.end(), bls.begin(), bls.end());
  }
  return results;
}

void SemanticLayer::print_semantic_info(bool detailed) const {
  std::cout << "=== semantic info ===" << std::endl;
  std::cout << "Total Nodes: " << semantic_nodes_.size() << std::endl;
  std::cout << "Total Edges: " << edges_.size() << std::endl;

  auto ref_nodes = query_nodes([](const MIGRNode &node) {
    return node.type_ == MIGRNodeType::REFERENCE;
  });
  auto tag_nodes = query_nodes(
      [](const MIGRNode &node) { return node.type_ == MIGRNodeType::TAG; });

  std::cout << "Reference nodes: " << ref_nodes.size() << std::endl;
  std::cout << "Tag Nodes: " << tag_nodes.size() << std::endl;

  if (!detailed) {
    return;
  }

  std::cout << "=== more detailed ===" << std::endl;
  if (!ref_nodes.empty()) {
    std::cout << "\n--- References ---" << std::endl;
    for (const auto &ref : ref_nodes) {
      auto target_it = ref->metadata_.find("target");
      auto type_it = ref->metadata_.find("link_type");

      std::string target =
          target_it != ref->metadata_.end() ? target_it->second : "[no target]";
      std::string link_type =
          type_it != ref->metadata_.end() ? type_it->second : "[unknown]";

      std::cout << "\nREF: " << ref->id_ << " -> '" << target << "' ("
                << link_type << ")" << std::endl;

      auto backlinks = find_backlinks(ref->id_);
      if (!backlinks.empty()) {
        std::cout << "  Backlinks:" << std::endl;
        for (const auto &bl : backlinks) {
          std::cout << "    " << bl->id_ << " <- " << bl->content_ << std::endl;
        }
      }
    }
  }

  if (!tag_nodes.empty()) {
    std::cout << "\n--- Tags ---" << std::endl;
    for (const auto &tag : tag_nodes) {
      auto tag_name_it = tag->metadata_.find("tag_name");
      std::string tag_name = tag_name_it != tag->metadata_.end()
                                 ? tag_name_it->second
                                 : "[no name]";

      std::cout << "\nTAG: " << tag->id_ << " -> '#" << tag_name << "'"
                << std::endl;

      auto backlinks = find_backlinks(tag->id_);
      if (!backlinks.empty()) {
        std::cout << "  Tagged by:" << std::endl;
        for (const auto &bl : backlinks) {
          std::cout << "    " << bl->id_ << " <- " << bl->content_ << std::endl;
        }
      }
    }
  }
  std::cout << std::endl;
}

void SemanticLayer::reset() {
  semantic_nodes_.clear();
  edges_.clear();
  backlink_index_.clear();
  reference_cache_.clear();
  tag_cache_.clear();
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

      auto ref_node = get_or_create_reference_node(target_url);

      add_semantic_edge(node, ref_node, MIGREdgeType::SEMANTIC_LINK,
                        "references");
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

      // confirming that it is a tag link and processing
      if (!url.empty() && url[0] == '#') {
        std::string tag_name = url.substr(1);

        auto tag_node = get_or_create_tag_node(tag_name);

        add_semantic_edge(node, tag_node, MIGREdgeType::TAG_RELATION,
                          "tagged_with");
      }
    }
  }

  // recursively processing the children
  for (const auto &child : node->children_) {
    extract_tags(child);
  }
}

void SemanticLayer::build_backlink_index() {
  backlink_index_.clear();
  for (const auto &edge : edges_) {
    backlink_index_[edge.target_id].push_back(edge.source_id);
  }
}

std::string SemanticLayer::classify_link_type(const std::string &target) const {
  if (target.find("http://") == 0 || target.find("https://") == 0) {
    return "external";
  }
  return "internal";
}

/* will prevent adding duplicate reference nodes, if node present just get it's
 * id, if not present then create and get it's id */
std::shared_ptr<MIGRNode>
SemanticLayer::get_or_create_reference_node(const std::string &target) {
  // checking cache first
  auto cache_it = reference_cache_.find(target);
  if (cache_it != reference_cache_.end()) {
    auto node_it = semantic_nodes_.find(cache_it->second);
    if (node_it != semantic_nodes_.end()) {
      return node_it->second;
    }
  }

  // creating new ref node
  auto ref_node = std::make_shared<MIGRNode>(MIGRNodeType::REFERENCE, target);
  ref_node->metadata_["target"] = target;
  ref_node->metadata_["link_type"] = classify_link_type(target);

  add_node(ref_node);
  reference_cache_[target] = ref_node->id_;

  return ref_node;
}

/* will prevent adding duplicate tag nodes, if node present just get it's id, if
 * not present then create and get it's id */
std::shared_ptr<MIGRNode>
SemanticLayer::get_or_create_tag_node(const std::string &tag_name) {
  // checking if already exists in cache
  auto cache_it = tag_cache_.find(tag_name);
  if (cache_it != tag_cache_.end()) {
    auto node_it = semantic_nodes_.find(cache_it->second);
    if (node_it != semantic_nodes_.end()) {
      return node_it->second;
    }
  }

  // creating new tagnode
  auto tag_node = std::make_shared<MIGRNode>(MIGRNodeType::TAG, tag_name);
  tag_node->metadata_["tag_name"] = tag_name;
  add_node(tag_node);
  tag_cache_[tag_name] = tag_node->id_;
  return tag_node;
}

#include "migr_semantic.h"
#include "globals.h"
#include <algorithm>
#include <memory>

//--------------------------//
//   MIGR Graph Interface   //
//--------------------------//

/*
 * Adds a node to the semantic_nodes map keyed by node id.
 * Meanwhile Ignoring null pointers.
 */
void SemanticLayer::add_node(std::shared_ptr<MIGRNode> node) {
  if (node) {
    semantic_nodes_[node->id_] = node;
  }
}

/*
 * Removes a node identified by node_id from semantic_nodes.
 * Also removes all edges connected to the node.
 * Cleans up semantic links in other nodes pointing to this node.
 * Clears references from the reference cache.
 * Rebuilds the backlink index after removal.
 */
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

  // cache clean
  for (auto cache_it = reference_cache_.begin();
       cache_it != reference_cache_.end();) {
    if (cache_it->second == node_id) {
      cache_it = reference_cache_.erase(cache_it);
    } else {
      ++cache_it;
    }
  }

  // removing from storage
  semantic_nodes_.erase(it);

  build_edge_indexes();
  build_backlink_index();
}

/*
 * Takes a predicate (true/false) function as input
 * Queries nodes by using that function.
 * Returns a vector of shared_ptrs containing nodes matching the predicate.
 */
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

/*
 * Serializes semantic_nodes and edges into JSON-like format.
 * Includes node ids, types, contents, metadata, and edge source/target and
 * relation details.
 */
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

/*
 * fix: Currently unimplemented
 */
void SemanticLayer::deserialize(std::istream &in) const {
  // todo: implement deserialization of data - would implement
  // json parsing as data will be in json format
  SPEAK << "Deserialization is not implemented yet" << std::endl;
}

//-----------------------------//
//     Semantic Operations     //
//-----------------------------//

/*
 * Extracts semantic information from the provided StructuralLayer.
 * Retrieves links -> [[link]] and tags -> [[#tag]], adds to semantic layer.
 * Builds backlink index.
 */
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

/*
 * Adds a semantic edge between source and target nodes.
 * Updates source node’s semantic links, so that this edge is there.
 * Registers the edge with type and relation label.
 */
void SemanticLayer::add_semantic_edge(const std::shared_ptr<MIGRNode> &source,
                                      const std::shared_ptr<MIGRNode> &target,
                                      MIGREdgeType edge_type,
                                      const std::string &relation_label) {
  size_t edge_idx = edges_.size();
  edges_.push_back({source->id_, target->id_, edge_type, relation_label});

  outgoing_edge_index_[source->id_].push_back(edge_idx);
  incoming_edge_index_[target->id_].push_back(edge_idx);
}

/*
 * Finds all nodes linking back to a target node by its id.
 * It utilizes backlink index for efficient lookup.
 */
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

/*
 * Searches for tag nodes matching a given tag string and their backlinks.
 * Returns vector that has nodes with tag and also it has nodes referencing
 * those tags.
 */
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

/*
 * Finds all reference nodes targeting a given name and their backlinks.
 */
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

//----------------------------//
//   Edge Query Operations    //
//----------------------------//

/*
 * Returns all target nodes that the give source node links to.
 * Uses outgoing edge index for O(1) lookup
 */
std::vector<std::shared_ptr<MIGRNode>>
SemanticLayer::get_semantic_targets(const std::string &source_id) const {
  std::vector<std::shared_ptr<MIGRNode>> targets;

  auto it = outgoing_edge_index_.find(source_id);
  if (it != outgoing_edge_index_.end()) {
    targets.reserve(it->second.size());
    for (size_t idx : it->second) {
      const auto &edge = edges_[idx];
      auto target_it = semantic_nodes_.find(edge.target_id);
      if (target_it != semantic_nodes_.end()) {
        targets.push_back(target_it->second);
      }
    }
  }
  return targets;
}

/*
 * Returns all source nodes that link to given target node.
 * Uses incoming edge index for O(1) lookup
 */
std::vector<std::shared_ptr<MIGRNode>>
SemanticLayer::get_semantic_sources(const std::string &target_id) const {
  std::vector<std::shared_ptr<MIGRNode>> sources;

  auto it = incoming_edge_index_.find(target_id);
  if (it != incoming_edge_index_.end()) {
    sources.reserve(it->second.size());
    for (size_t idx : it->second) {
      const auto &edge = edges_[idx];
      auto source_it = semantic_nodes_.find(edge.source_id);
      if (source_it != semantic_nodes_.end()) {
        sources.push_back(source_it->second);
      }
    }
  }
  return sources;
}

/*
 * Return all edges originating from a given semantic node.
 */
std::vector<SemanticEdge>
SemanticLayer::get_edges_from_node(const std::string &node_id) const {
  std::vector<SemanticEdge> result;

  auto it = outgoing_edge_index_.find(node_id);
  if (it != outgoing_edge_index_.end()) {
    result.reserve(it->second.size());
    for (size_t idx : it->second) {
      result.push_back(edges_[idx]);
    }
  }
  return result;
}

/*
 * Return all edges going into a given semantic node.
 */
std::vector<SemanticEdge>
SemanticLayer::get_edges_to_node(const std::string &node_id) const {
  std::vector<SemanticEdge> result;

  auto it = incoming_edge_index_.find(node_id);
  if (it != incoming_edge_index_.end()) {
    result.reserve(it->second.size());
    for (size_t idx : it->second) {
      result.push_back(edges_[idx]);
    }
  }
  return result;
}

//-------------------//
//   For Debugging   //
//-------------------//

/*
 * Prints semantic summary which has info of:
 * total nodes, edges, and counts of references and tags.
 * If param detailed is true, then prints extended info
 * about each reference and tag with backlinks.
 */
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

//-------------------//
//      Helpers      //
//-------------------//

/*
 * Resets the semantic layer state by clearing nodes,
 * edges, backlink index, and caches.
 */
void SemanticLayer::reset() {
  semantic_nodes_.clear();
  edges_.clear();
  backlink_index_.clear();
  reference_cache_.clear();
  tag_cache_.clear();
}

/*
 * Recursively extracts semantic links from the given node.
 * Processes LINK type nodes, skips tag links (starting with '#').
 * Adds semantic edges from link nodes to reference nodes.
 */
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

/*
 * Recursively extracts tags from the given node.
 * Processes LINK nodes with URLs starting with '#'.
 * Adds semantic edges between the linking node and tag node.
 */
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

/*
 * Builds backlink index mapping target node ids to source node ids.
 * The created cache enables fast O(1) reverse lookup of backlinks.
 */
void SemanticLayer::build_backlink_index() {
  backlink_index_.clear();
  for (const auto &edge : edges_) {
    backlink_index_[edge.target_id].push_back(edge.source_id);
  }
}

/*
 * Builds edge indexes mapping node IDs to their outgoing and incoming edge
 * indices.
 * Enables O(1) lookup of edges connected to a given node.
 */
void SemanticLayer::build_edge_indexes() {
  outgoing_edge_index_.clear();
  incoming_edge_index_.clear();

  for (size_t i{0}; i < edges_.size(); ++i) {
    const auto &edge = edges_[i];
    outgoing_edge_index_[edge.source_id].push_back(i);
    incoming_edge_index_[edge.target_id].push_back(i);
  }
}

/*
 * Based on whether the target string starts with "http://" or "https://".
 * It classifies a link target as "external" or "internal".
 */
std::string SemanticLayer::classify_link_type(const std::string &target) const {
  if (target.find("http://") == 0 || target.find("https://") == 0) {
    return "external";
  }
  return "internal";
}

//------------------------//
//    Node Management     //
//------------------------//

/*
 * Returns existing reference node for target if cached,
 * otherwise creates new one.
 * Then Caches that nodes to prevent duplicates.
 */
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

/*
 * Returns existing tag node for tag_name if cached,
 * otherwise creates new one.
 * Then Caches that nodes to prevent duplicates.
 */
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

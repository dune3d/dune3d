#pragma once

#include <set>
#include <list>
#include <functional>
#include <deque>
#include <glm/glm.hpp>


namespace dune3d {

class Entity;
class EntityCircle2D;
class Document;
class UUID;

namespace paths {

class Node {
public:
    Node(const glm::dvec2 &ap);
    glm::dvec2 p;
    std::set<std::pair<class Edge *, unsigned int>> connected_edges;
    std::array<Edge *, 2> get_edges();
    bool is_valid() const;
    unsigned int get_pt_for_edge(const Edge &edge) const;
    unsigned int tag = 0;
};


class Edge {
public:
    using Transform = std::function<glm::dvec2(glm::dvec2)>;
    Edge(std::list<Node> &nodes, const Entity &e, Transform tr);
    Edge(Node &node, const EntityCircle2D &e, Transform tr);
    Node &from;
    Node &to;
    Node &get_other_node(Node &node);
    const Entity &entity;
    const Transform transform_fn;
    glm::dvec2 transform(const glm::dvec2 &v) const;
};

using Path = std::deque<std::pair<Node &, Edge &>>;

class Paths {
public:
    std::deque<Path> paths;
    static Paths from_document(const Document &doc, const UUID &wrkpl_uu, const UUID &source_group_uu);
    static glm::dvec2 get_pt(const Entity &e, unsigned int pt, Edge::Transform tr);

private:
    std::list<Node> nodes;
    std::list<Edge> edges;
};
} // namespace paths
} // namespace dune3d

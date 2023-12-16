#pragma once
#include <glm/glm.hpp>
#include <deque>
#include <set>
#include <list>
#include "clipper2/clipper.h"
#include <TopoDS_Builder.hxx>


namespace dune3d {

class UUID;
class Document;
class EntityWorkplane;
class EntityCircle2D;
class Entity;

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
    Edge(std::list<Node> &nodes, const Entity &e);
    Edge(Node &node, const EntityCircle2D &e);
    Node &from;
    Node &to;
    Node &get_other_node(Node &node);
    const Entity &entity;
};

using Path = std::deque<std::pair<Node &, Edge &>>;

class FaceBuilder {
public:
    static FaceBuilder from_document(const Document &doc, const UUID &wrkpl_uu, const UUID &source_group_uu,
                                     const glm::dvec3 &offset);

    const TopoDS_Compound &get_faces() const;
    unsigned int get_n_faces() const;

private:
    FaceBuilder(const EntityWorkplane &wrkpl, const std::deque<Path> &paths, const glm::dvec3 &offset);

    static bool check_path(const Clipper2Lib::PathD &contour);

    void visit_poly_path(const Clipper2Lib::PolyPathD &path);

    const EntityWorkplane &m_wrkpl;
    const std::deque<Path> &m_paths;
    const glm::dvec3 m_offset;
    TopoDS_Compound m_compound;
    TopoDS_Builder m_builder;
    unsigned int m_n_faces = 0;

    TopoDS_Wire path_to_wire(const Clipper2Lib::PathD &path, bool hole);
};


} // namespace dune3d

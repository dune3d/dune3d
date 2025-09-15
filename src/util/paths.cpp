#include "paths.hpp"
#include <stdexcept>
#include "document/entity/ientity_in_workplane.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_circle2d.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_cluster.hpp"
#include "document/entity/entity_text.hpp"
#include "document/document.hpp"

namespace dune3d::paths {

Node::Node(const glm::dvec2 &ap) : p(ap)
{
}
bool Node::is_valid() const
{
    return connected_edges.size() == 2;
}
unsigned int Node::get_pt_for_edge(const Edge &edge) const
{
    for (const auto &[e, pt] : connected_edges) {
        if (e == &edge)
            return pt;
    }
    throw std::runtime_error("not an edge of node");
}

static Node &get_or_create_node(std::list<Node> &nodes, const glm::dvec2 &p)
{
    for (auto &node : nodes) {
        if (glm::length(node.p - p) < 1e-6)
            return node;
    }
    return nodes.emplace_back(p);
}


glm::dvec2 Paths::get_pt(const Entity &e, unsigned int pt, Edge::Transform tr)
{
    auto &en_wrkpl = dynamic_cast<const IEntityInWorkplane &>(e);
    auto p = en_wrkpl.get_point_in_workplane(pt);
    if (tr)
        return tr(p);
    else
        return p;
}

Edge::Edge(std::list<Node> &nodes, const Entity &e, Transform tr)
    : from(get_or_create_node(nodes, Paths::get_pt(e, 1, tr))), to(get_or_create_node(nodes, Paths::get_pt(e, 2, tr))),
      entity(e), transform_fn(tr)
{
    from.connected_edges.emplace(this, 1);
    to.connected_edges.emplace(this, 2);
}

Edge::Edge(Node &node, const EntityCircle2D &e, Transform tr) : from(node), to(node), entity(e), transform_fn(tr)
{
}

Node &Edge::get_other_node(Node &node)
{
    if (&node == &from)
        return to;
    else if (&node == &to)
        return from;
    assert(false);
}

glm::dvec2 Edge::transform(const glm::dvec2 &v) const
{
    if (transform_fn)
        return transform_fn(v);
    else
        return v;
}

std::array<Edge *, 2> Node::get_edges()
{
    assert(connected_edges.size() == 2);
    auto it = connected_edges.begin();
    auto &e1 = *it++;
    auto &e2 = *it;
    if (e1.first->entity.m_uuid < e2.first->entity.m_uuid)
        return {e1.first, e2.first};
    else
        return {e2.first, e1.first};
}


static bool entity_is_valid(const Entity &en, Edge::Transform tr)
{
    if (auto en_line = dynamic_cast<const EntityLine2D *>(&en))
        if (glm::length(tr(en_line->m_p1) - tr(en_line->m_p2)) < 1e-6)
            return false;
    return true;
}

Paths Paths::from_document(const Document &doc, const UUID &wrkpl_uu, const UUID &source_group_uu)
{
    Paths paths;
    for (const auto &[uu, en] : doc.m_entities) {
        if (en->m_group != source_group_uu)
            continue;
        if (en->m_construction)
            continue;
        if (en->get_type() == Entity::Type::CIRCLE_2D)
            continue;
        if (en->get_type() == Entity::Type::POINT_2D)
            continue;
        if (auto en_wrkpl = dynamic_cast<const IEntityInWorkplane *>(en.get())) {
            if (en_wrkpl->get_workplane() != wrkpl_uu)
                continue;
            if (!entity_is_valid(*en, [](const auto &x) { return x; }))
                continue;
            if (auto en_cluster = dynamic_cast<const EntityCluster *>(en.get())) {
                auto tr = [en_cluster](const glm::dvec2 &v) { return en_cluster->transform(v); };
                for (const auto &[uu2, en2] : en_cluster->m_content->m_entities) {
                    if (en2->m_construction)
                        continue;
                    if (!entity_is_valid(*en2, tr))
                        continue;
                    if (en2->of_type(Entity::Type::CIRCLE_2D))
                        continue;

                    paths.edges.emplace_back(paths.nodes, *en2, tr);
                }
            }
            else if (auto en_text = dynamic_cast<const EntityText *>(en.get())) {
                auto tr = [en_text](const glm::dvec2 &v) { return en_text->transform(v); };
                for (const auto &[uu2, en2] : en_text->m_content->m_entities) {
                    if (!entity_is_valid(*en2, tr))
                        continue;
                    if (en2->of_type(Entity::Type::CIRCLE_2D))
                        continue;

                    paths.edges.emplace_back(paths.nodes, *en2, tr);
                }
            }
            else {
                paths.edges.emplace_back(paths.nodes, *en, nullptr);
            }
        }
    }

    int tag = 1;
    while (true) {
        // find a node with tag 0, i.e. hasn't been visited yet
        auto it = std::ranges::find_if(paths.nodes, [](auto &x) { return x.tag == 0 && x.is_valid(); });
        if (it == paths.nodes.end())
            break;
        auto node = &(*it);
        node->tag = tag++;
        Path path;
        do {
            auto edges = node->get_edges();
            auto last_node = node;
            for (auto edge : edges) {
                if (path.size() && (&path.back().second == edge))
                    continue;
                auto &next_node = edge->get_other_node(*node);

                if (!next_node.is_valid())
                    continue;
                path.emplace_back(*node, *edge);
                if (&next_node == node) {
                    node = nullptr;
                    break;
                }
                if (next_node.tag == 0) {
                    // no one was here yet
                    next_node.tag = node->tag;
                    node = &next_node;
                    break;
                }
                else if (next_node.tag == node->tag) {
                    // found cycle
                    node = nullptr;
                    break;
                }
                else { // next_node.tag != start_node->tag
                    path.clear();
                    node = nullptr;
                    break;
                    // this should not happen
                }
            }
            if (last_node == node) {
                node = nullptr;
                path.clear();
            }
        } while (node);
        if (path.size())
            paths.paths.emplace_back(std::move(path));
    }

    // add circles
    for (const auto &[uu, en] : doc.m_entities) {
        if (en->m_group != source_group_uu)
            continue;
        if (en->m_construction)
            continue;
        if (en->of_type(Entity::Type::CIRCLE_2D)) {
            auto &circle = dynamic_cast<const EntityCircle2D &>(*en);
            if (circle.get_workplane() != wrkpl_uu)
                continue;

            auto &node = paths.nodes.emplace_back(circle.m_center);
            auto &edge = paths.edges.emplace_back(node, circle, nullptr);
            Path path;
            path.emplace_back(node, edge);
            paths.paths.emplace_back(std::move(path));
        }
        else if (auto en_cluster = dynamic_cast<const EntityCluster *>(en.get())) {
            auto tr = [en_cluster](const glm::dvec2 &v) { return en_cluster->transform(v); };
            for (const auto &[uu2, en2] : en_cluster->m_content->m_entities) {
                if (en2->m_construction)
                    continue;
                if (en2->of_type(Entity::Type::CIRCLE_2D)) {
                    auto &circle = dynamic_cast<const EntityCircle2D &>(*en2);
                    auto &node = paths.nodes.emplace_back(en_cluster->transform(circle.m_center));
                    auto &edge = paths.edges.emplace_back(node, circle, tr);
                    Path path;
                    path.emplace_back(node, edge);
                    paths.paths.emplace_back(std::move(path));
                }
            }
        }
    }
    return paths;
}


} // namespace dune3d::paths

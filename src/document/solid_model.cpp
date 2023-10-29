#include "solid_model.hpp"
#include "BRepAdaptor_Curve.hxx"
#include "solid_model_occ.hpp"
#include "document.hpp"
#include "entity/entity_line2d.hpp"
#include "entity/entity_arc2d.hpp"
#include "entity/entity_circle2d.hpp"
#include "entity/entity_workplane.hpp"
#include "group/group_extrude.hpp"
#include "group/group_fillet.hpp"
#include "group/group_chamfer.hpp"
#include "group/group_lathe.hpp"
#include "util/glm_util.hpp"
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepBuilderAPI.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>

#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>

#include <BOPAlgo_Tools.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <TopoDS_Builder.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_TangentialDeflection.hxx>


#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include "clipper2/clipper.h"
#include <set>
#include <list>
#include <ranges>
#include <gp_Ax3.hxx>
#include <gp_Pln.hxx>

#include <format>

namespace dune3d {

SolidModel::~SolidModel() = default;

class Node {
public:
    Node(const glm::dvec2 &ap) : p(ap)
    {
    }
    glm::dvec2 p;
    std::set<std::pair<class Edge *, unsigned int>> connected_edges;
    std::array<Edge *, 2> get_edges();
    bool is_valid() const
    {
        return connected_edges.size() == 2;
    }
    unsigned int get_pt_for_edge(const Edge &edge) const
    {
        for (const auto &[e, pt] : connected_edges) {
            if (e == &edge)
                return pt;
        }
        throw std::runtime_error("not an edge of node");
    }
    unsigned int tag = 0;
};

static Node &get_or_create_node(std::list<Node> &nodes, const glm::dvec2 &p)
{
    for (auto &node : nodes) {
        if (glm::length(node.p - p) < 1e-6)
            return node;
    }
    return nodes.emplace_back(p);
}


static glm::dvec2 get_pt(const Entity &e, unsigned int pt)
{
    if (e.get_type() == Entity::Type::LINE_2D) {
        auto &line = dynamic_cast<const EntityLine2D &>(e);
        if (pt == 1)
            return line.m_p1;
        else
            return line.m_p2;
    }
    else if (e.get_type() == Entity::Type::ARC_2D) {
        auto &line = dynamic_cast<const EntityArc2D &>(e);
        if (pt == 1)
            return line.m_from;
        else
            return line.m_to;
    }
    throw std::runtime_error("unexpected entity");
}

class Edge {
public:
    Edge(std::list<Node> &nodes, const Entity &e)
        : from(get_or_create_node(nodes, get_pt(e, 1))), to(get_or_create_node(nodes, get_pt(e, 2))), entity(e)
    {
        from.connected_edges.emplace(this, 1);
        to.connected_edges.emplace(this, 2);
    }
    Edge(Node &node, const EntityCircle2D &e) : from(node), to(node), entity(e)
    {
    }
    Node &from;
    Node &to;
    Node &get_other_node(Node &node)
    {
        if (&node == &from)
            return to;
        else if (&node == &to)
            return from;
        assert(false);
    }
    const Entity &entity;
};

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

using Path = std::deque<std::pair<Node &, Edge &>>;

static double angle(const glm::dvec2 &v)
{
    return glm::atan(v.y, v.x);
}

template <typename T> T c2pi(T x)
{
    while (x < 0)
        x += 2 * M_PI;

    while (x > 2 * M_PI)
        x -= 2 * M_PI;
    return x;
}

template <typename T> T c2pim(T x)
{
    while (x < -2 * M_PI)
        x += 2 * M_PI;

    while (x > 0)
        x -= 2 * M_PI;
    return x;
}

static glm::dvec2 euler(double r, double phi)
{
    return {r * cos(phi), r * sin(phi)};
}

struct VertexInfo {

    bool sub;
    unsigned int vertex_index;
    unsigned int path_index;

    static constexpr unsigned int mask = (1u << 31) - 1;

    constexpr int64_t pack() const
    {
        assert(vertex_index <= mask);
        assert(path_index <= mask);
        return sub | ((vertex_index & mask) << 1) | (static_cast<uint64_t>(path_index & mask) << (31 + 1));
    }

    int64_t get_vertex_index() const
    {
        return sub | (vertex_index << 1);
    }

    static constexpr VertexInfo unpack(int64_t z)
    {
        auto s = static_cast<bool>(z & 1);
        auto v = static_cast<unsigned int>((z >> 1) & (mask));
        auto p = static_cast<unsigned int>((z >> (31 + 1)) & (mask));
        return {s, v, p};
    }

    static constexpr int64_t make_z(unsigned int path_index, unsigned int vertex_index, unsigned int i,
                                    unsigned int segments)
    {
        return VertexInfo{.sub = i > segments / 2, .vertex_index = vertex_index, .path_index = path_index}.pack();
    }
};


static Clipper2Lib::PathD path_to_clipper(const Path &path, unsigned int path_index)
{
    Clipper2Lib::PathD cpath;
    cpath.reserve(path.size());
    for (size_t iv = 0; iv < path.size(); iv++) {
        auto &[node, edge] = path.at(iv);
        if (auto circle = dynamic_cast<const EntityCircle2D *>(&edge.entity)) {
            {
                const unsigned int segments = 8;

                float dphi = 2 * M_PI;
                dphi /= segments;
                float a = 0;
                for (unsigned int i = 0; i < segments; i++) {
                    const auto p0 = circle->m_center + euler(circle->m_radius, a);
                    cpath.emplace_back(p0.x, p0.y, VertexInfo::make_z(path_index, iv, i, segments));
                    a += dphi;
                }
            }
            break;
        }


        auto pt = node.get_pt_for_edge(edge);
        auto pc = get_pt(edge.entity, pt);


        if (auto arc = dynamic_cast<const EntityArc2D *>(&edge.entity)) {
            const auto radius0 = glm::length(arc->m_center - arc->m_from);
            const auto a0 = c2pi(angle(pc - arc->m_center));
            const auto a1 = c2pi(angle(get_pt(edge.entity, pt == 1 ? 2 : 1) - arc->m_center));
            const unsigned int segments = 64;

            float dphi = c2pi(a1 - a0);
            if (pt == 2) {
                dphi = c2pim(a1 - a0);
            }
            if (std::abs(dphi) < 1e-2)
                dphi = 2 * M_PI;

            dphi /= segments;
            float a = a0;
            for (unsigned int i = 0; i < segments; i++) {
                const auto p0 = arc->m_center + euler(radius0, a);
                cpath.emplace_back(p0.x, p0.y, VertexInfo::make_z(path_index, iv, i, segments));
                a += dphi;
            }
        }
        else {
            cpath.emplace_back(
                    pc.x, pc.y,
                    VertexInfo{.sub = false, .vertex_index = static_cast<unsigned int>(iv), .path_index = path_index}
                            .pack());
        }
    }

    return cpath;
}


class FaceBuilder {
public:
    FaceBuilder(const EntityWorkplane &wrkpl, const std::deque<Path> &paths, const glm::dvec3 &offset)
        : m_wrkpl(wrkpl), m_paths(paths), m_offset(offset)
    {
        m_builder.MakeCompound(m_compound);
    }


    static bool check_path(const Clipper2Lib::PathD &contour)
    {
        const auto path_index = VertexInfo::unpack(contour.front().z).path_index;
        if (contour.front().z == -1)
            return false;
        auto same_z = std::ranges::all_of(contour, [path_index](auto &pt) {
            return VertexInfo::unpack(pt.z).path_index == path_index && pt.z != -1;
        });
        if (!same_z)
            return false;
        return true;
    }

    void visit_poly_path(const Clipper2Lib::PolyPathD &path)
    {
        assert(!path.IsHole());

        auto &contour = path.Polygon();
        if (!check_path(contour))
            return;

        auto wire = path_to_wire(contour, false);
        BRepBuilderAPI_MakeFace make_face(wire);

        for (auto &child : path) {
            auto &hole = *child;
            assert(hole.IsHole());

            auto &hole_contour = hole.Polygon();
            if (check_path(hole_contour)) {
                auto hole_wire = path_to_wire(hole_contour, true);
                make_face.Add(hole_wire);
            }
            //   visit_poly_path(*poly_tree.Child(i));
        }
        // make_face.Add()

        //
        // make_face.Face()
        m_n_faces++;
        m_builder.Add(m_compound, make_face.Face());
    }

    auto &get_faces() const
    {
        return m_compound;
    }

    auto get_n_faces() const
    {
        return m_n_faces;
    }

private:
    const EntityWorkplane &m_wrkpl;
    const std::deque<Path> &m_paths;
    const glm::dvec3 m_offset;
    TopoDS_Compound m_compound;
    TopoDS_Builder m_builder;
    unsigned int m_n_faces = 0;

    static std::pair<const Node &, const Edge &> get_node_and_edge(const Path &path, size_t i, bool reverse)
    {
        if (!reverse)
            return path.at(i);

        i = path.size() - i - 1;
        size_t iprev = i - 1;
        if (i == 0)
            iprev = path.size() - 1;
        return {path.at(i).first, path.at(iprev).second};
    }

    TopoDS_Wire path_to_wire(const Clipper2Lib::PathD &path, bool hole)
    {

        Path orig_path = m_paths.at(VertexInfo::unpack(path.front().z).path_index);
        if (orig_path.size() == 1) {
            BRepBuilderAPI_MakeWire wire;
            auto &rad = dynamic_cast<const IEntityRadius &>(orig_path.front().second.entity);
            const auto center = m_wrkpl.transform(rad.get_center()) + m_offset;

            auto normal = m_wrkpl.get_normal();
            if (hole)
                normal *= -1;

            gp_Circ garc(gp_Ax2(gp_Pnt(center.x, center.y, center.z), gp_Dir(normal.x, normal.y, normal.z)),
                         rad.get_radius());


            auto edge = BRepBuilderAPI_MakeEdge(garc);
            wire.Add(edge);

            return wire;
        }


        assert(path.size() >= 3);
        // see if we're traversing orig_path in reverse

        const auto sz = path.size();

        unsigned int pos_count = 0;
        unsigned int neg_count = 0;

        for (size_t i = 0; i < sz; i++) {
            auto inext = (i + 1) % sz;
            auto vi = VertexInfo::unpack(path.at(i).z);
            auto vinext = VertexInfo::unpack(path.at(inext).z);
            auto delta = vinext.get_vertex_index() - vi.get_vertex_index();
            if (delta > 0)
                pos_count++;
            else if (delta < 0)
                neg_count++;
            if (pos_count + neg_count >= 3)
                break;
        }
        const bool path_reverse = neg_count > pos_count;

        BRepBuilderAPI_MakeWire wire;


        for (size_t i = 0; i < orig_path.size(); i++) {
            const auto &[node, edge] = get_node_and_edge(orig_path, i, path_reverse);
            const auto pt = node.get_pt_for_edge(edge);
            const auto next_pt = pt == 1 ? 2 : 1;
            const auto pa = get_pt(edge.entity, pt);
            const auto pb = get_pt(edge.entity, next_pt);

            const auto pat = m_wrkpl.transform(pa) + m_offset;
            const auto pbt = m_wrkpl.transform(pb) + m_offset;

            if (auto arc = dynamic_cast<const EntityArc2D *>(&edge.entity)) {
                auto normal = m_wrkpl.get_normal();

                gp_Pnt sa(pat.x, pat.y, pat.z);
                gp_Pnt ea(pbt.x, pbt.y, pbt.z);
                const auto center = m_wrkpl.transform(arc->m_center) + m_offset;
                const auto radius = arc->get_radius();
                if (pt == 2)
                    normal *= -1;

                gp_Circ garc(gp_Ax2(gp_Pnt(center.x, center.y, center.z), gp_Dir(normal.x, normal.y, normal.z)),
                             radius);

                auto edge = BRepBuilderAPI_MakeEdge(garc, sa, ea);
                wire.Add(edge);
            }
            else {
                auto edge = BRepBuilderAPI_MakeEdge(gp_Pnt(pat.x, pat.y, pat.z), gp_Pnt(pbt.x, pbt.y, pbt.z));

                wire.Add(edge);
            }
        }
        return wire;
    }
};

static bool path_is_valid(const Path &path)
{
    if (path.size() >= 3)
        return true;
    if (path.size() == 1 && path.front().second.entity.get_type() == Entity::Type::CIRCLE_2D)
        return true;
    for (auto &p : path) {
        if (p.second.entity.get_type() == Entity::Type::ARC_2D)
            return true;
    }

    return false;
}

const SolidModel *SolidModel::get_last_solid_model(const Document &doc, const Group &group)
{
    const SolidModelOcc *last_solid_model = nullptr;

    auto this_body = &group.find_body(doc).body;

    for (auto gr : doc.get_groups_sorted()) {
        if (gr->m_uuid == group.m_uuid)
            break;
        if (auto gr_solid = dynamic_cast<const IGroupSolidModel *>(gr)) {
            if (auto solid_model = dynamic_cast<const SolidModelOcc *>(gr_solid->get_solid_model())) {
                auto body = &gr->find_body(doc).body;
                if (body != this_body)
                    continue;
                if (!solid_model->m_shape_acc.IsNull())
                    last_solid_model = solid_model;
            }
        }
    }

    return last_solid_model;
}


static FaceBuilder faces_from_document(const Document &doc, const UUID &wrkpl_uu, const UUID &source_group_uu,
                                       const glm::dvec3 &offset)
{
    std::list<Node> nodes;
    std::list<Edge> edges;
    for (const auto &[uu, en] : doc.m_entities) {
        if (en->m_group != source_group_uu)
            continue;
        if (en->m_construction)
            continue;
        if (en->get_type() == Entity::Type::CIRCLE_2D)
            continue;
        if (auto en_wrkpl = dynamic_cast<const IEntityInWorkplane *>(en.get())) {
            if (en_wrkpl->get_workplane() != wrkpl_uu)
                continue;
            edges.emplace_back(nodes, *en);
        }
    }

    // find closed paths
    std::deque<Path> paths;

    int tag = 1;
    while (true) {
        // find a node with tag 0, i.e. hasn't been visited yet
        auto it = std::ranges::find_if(nodes, [](auto &x) { return x.tag == 0 && x.is_valid(); });
        if (it == nodes.end())
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
                    // this should not happen
                }
            }
            if (last_node == node) {
                node = nullptr;
                path.clear();
            }
        } while (node);
        if (path.size())
            paths.emplace_back(std::move(path));
    }

    // add circles
    for (const auto &[uu, en] : doc.m_entities) {
        if (en->m_group != source_group_uu)
            continue;
        if (en->m_construction)
            continue;
        if (en->get_type() != Entity::Type::CIRCLE_2D)
            continue;
        auto &circle = dynamic_cast<const EntityCircle2D &>(*en);
        if (circle.get_workplane() != wrkpl_uu)
            continue;

        auto &node = nodes.emplace_back(circle.m_center);
        auto &edge = edges.emplace_back(node, circle);
        Path path;
        path.emplace_back(node, edge);
        paths.emplace_back(std::move(path));
    }


    // have node/edge paths, now turn them into clipper paths
    Clipper2Lib::PathsD cpaths;
    cpaths.reserve(paths.size());
    {
        unsigned int path_index = 0;
        for (auto &path : paths) {
            if (path_is_valid(path))
                cpaths.emplace_back(path_to_clipper(path, path_index++));
        }
    }
    Clipper2Lib::PolyTreeD poly_tree;
    Clipper2Lib::ClipperD clipper;
    clipper.AddSubject(cpaths);
    if (0) {
        std::ofstream ofs("/tmp/paths.txt");
        for (auto &path : cpaths) {
            for (auto &p : path) {
                ofs << p.x << " " << p.y << " " << p.z << std::endl;
            }
            ofs << path.front().x << " " << path.front().y << " " << path.front().z << std::endl;
            ofs << std::endl;
        }
    }
    clipper.SetZCallback([](const Clipper2Lib::PointD &e1bot, const Clipper2Lib::PointD &e1top,
                            const Clipper2Lib::PointD &e2bot, const Clipper2Lib::PointD &e2top,
                            Clipper2Lib::PointD &pt) { pt.z = -1; });

    clipper.Execute(Clipper2Lib::ClipType::Union, Clipper2Lib::FillRule::EvenOdd, poly_tree);

    auto &wrkpl = doc.get_entity<EntityWorkplane>(wrkpl_uu);

    FaceBuilder face_builder{wrkpl, paths, offset};

    for (auto &child : poly_tree) {
        face_builder.visit_poly_path(*child);
    }

    return face_builder;
}

std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, const GroupExtrude &group)
{
    auto mod = std::make_shared<SolidModelOcc>();


    glm::dvec3 offset = {0, 0, 0};
    glm::dvec3 dvec = group.m_dvec;

    switch (group.m_mode) {
    case GroupExtrude::Mode::SINGLE:
        break;
    case GroupExtrude::Mode::OFFSET_SYMMETRIC:
        offset = -group.m_dvec;
        dvec = group.m_dvec * 2.;
        break;
    case GroupExtrude::Mode::OFFSET:
        offset = group.m_dvec * group.m_offset_mul;
        dvec = group.m_dvec * (1 - group.m_offset_mul);
        break;
    }

    FaceBuilder face_builder = faces_from_document(doc, group.m_wrkpl, group.m_source_group, offset);

    if (face_builder.get_n_faces() == 0)
        return nullptr;

    if (glm::length(dvec) < 1e-6)
        return nullptr;

    mod->m_shape = BRepPrimAPI_MakePrism(face_builder.get_faces(), gp_Vec(dvec.x, dvec.y, dvec.z));

    const auto last_solid_model = dynamic_cast<const SolidModelOcc *>(get_last_solid_model(doc, group));

    if (last_solid_model) {
        switch (group.m_operation) {
        case GroupExtrude::Operation::DIFFERENCE:
            mod->m_shape_acc = BRepAlgoAPI_Cut(last_solid_model->m_shape_acc, mod->m_shape);
            break;
        case GroupExtrude::Operation::UNION:
            mod->m_shape_acc = BRepAlgoAPI_Fuse(last_solid_model->m_shape_acc, mod->m_shape);
            break;
        }
    }
    else {
        mod->m_shape_acc = mod->m_shape;
    }

    mod->find_edges();

    mod->triangulate();

    return mod;
}

std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, const GroupLathe &group)
{
    auto mod = std::make_shared<SolidModelOcc>();


    glm::dvec3 offset = {0, 0, 0};


    FaceBuilder face_builder = faces_from_document(doc, group.m_wrkpl, group.m_source_group, offset);

    if (face_builder.get_n_faces() == 0)
        return nullptr;

    auto origin = doc.get_entity(group.m_origin).get_point(group.m_origin_point, doc);

    glm::dvec3 dir;
    {
        const auto &en_normal = doc.get_entity(group.m_normal);
        const auto en_type = en_normal.get_type();
        if (auto wrkpl = dynamic_cast<const EntityWorkplane *>(&en_normal)) {
            dir = wrkpl->get_normal();
        }
        else if (en_type == Entity::Type::LINE_2D || en_type == Entity::Type::LINE_3D) {
            dir = glm::normalize(en_normal.get_point(2, doc) - en_normal.get_point(1, doc));
        }
        else {
            return nullptr;
        }
    }

    gp_Ax1 ax{gp_Pnt(origin.x, origin.y, origin.z), gp_Dir(dir.x, dir.y, dir.z)};


    BRepPrimAPI_MakeRevol mr{face_builder.get_faces(), ax};

    mr.Build();
    if (!mr.IsDone())
        return nullptr;

    mod->m_shape = mr.Shape();


    const auto last_solid_model = dynamic_cast<const SolidModelOcc *>(get_last_solid_model(doc, group));

    if (last_solid_model) {
        switch (group.m_operation) {
        case GroupLathe::Operation::DIFFERENCE:
            mod->m_shape_acc = BRepAlgoAPI_Cut(last_solid_model->m_shape_acc, mod->m_shape);
            break;
        case GroupLathe::Operation::UNION:
            mod->m_shape_acc = BRepAlgoAPI_Fuse(last_solid_model->m_shape_acc, mod->m_shape);
            break;
        }
    }
    else {
        mod->m_shape_acc = mod->m_shape;
    }

    mod->find_edges();

    mod->triangulate();

    return mod;
}


std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, const GroupFillet &group)
{
    if (group.m_edges.size() == 0)
        return nullptr;

    auto mod = std::make_shared<SolidModelOcc>();

    const auto last_solid_model = dynamic_cast<const SolidModelOcc *>(get_last_solid_model(doc, group));
    if (!last_solid_model)
        return nullptr;

    BRepFilletAPI_MakeFillet mf(last_solid_model->m_shape_acc);
    {
        TopExp_Explorer topex(last_solid_model->m_shape_acc, TopAbs_EDGE);
        std::list<TopoDS_Shape> edges;
        unsigned int edge_idx = 0;
        while (topex.More()) {
            auto edge = TopoDS::Edge(topex.Current());

            if (group.m_edges.contains(edge_idx)) {
                mf.Add(group.m_radius, edge);
            }

            topex.Next();
            edge_idx++;
        }
    }


    mf.Build();
    if (!mf.IsDone())
        return nullptr;

    mod->m_shape_acc = mf.Shape();

    mod->find_edges();
    mod->triangulate();
    return mod;
}

std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, const GroupChamfer &group)
{
    if (group.m_edges.size() == 0)
        return nullptr;

    auto mod = std::make_shared<SolidModelOcc>();

    const auto last_solid_model = dynamic_cast<const SolidModelOcc *>(get_last_solid_model(doc, group));
    if (!last_solid_model)
        return nullptr;

    BRepFilletAPI_MakeChamfer mf(last_solid_model->m_shape_acc);
    {
        TopExp_Explorer topex(last_solid_model->m_shape_acc, TopAbs_EDGE);
        std::list<TopoDS_Shape> edges;
        unsigned int edge_idx = 0;
        while (topex.More()) {
            auto edge = TopoDS::Edge(topex.Current());

            if (group.m_edges.contains(edge_idx)) {
                mf.Add(group.m_radius, edge);
            }

            topex.Next();
            edge_idx++;
        }
    }


    mf.Build();
    if (!mf.IsDone())
        return nullptr;

    mod->m_shape_acc = mf.Shape();

    mod->find_edges();
    mod->triangulate();
    return mod;
}

} // namespace dune3d

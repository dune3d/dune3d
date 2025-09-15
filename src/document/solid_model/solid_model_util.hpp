#pragma once
#include <glm/glm.hpp>
#include <deque>
#include <set>
#include <list>
#include "clipper2/clipper.h"
#include <TopoDS_Builder.hxx>
#include <TopoDS_Edge.hxx>


namespace dune3d {

class UUID;
class Document;
class EntityWorkplane;
class EntityCircle2D;
class Entity;

namespace paths {
class Paths;
}
namespace solid_model_util {
using namespace paths;

class FaceBuilder {
public:
    using Transform = std::function<glm::dvec3(const glm::dvec3 &)>;
    static FaceBuilder from_document(const Document &doc, const UUID &wrkpl_uu, const UUID &source_group_uu,
                                     const glm::dvec3 &offset);
    static FaceBuilder from_document(const Document &doc, const UUID &wrkpl_uu, const UUID &source_group_uu,
                                     Transform fn_transform, Transform fn_transform_normal);

    const TopoDS_Compound &get_faces() const;
    const auto &get_wires() const
    {
        return m_wires;
    }

    bool has_hole() const
    {
        return m_has_hole;
    }

    unsigned int get_n_faces() const;

private:
    FaceBuilder();

    struct DocumentRef {
        const EntityWorkplane &wrkpl;
        const paths::Paths &paths;
        Transform transform;
        Transform transform_normal;
    };

    static bool check_path(const Clipper2Lib::PathD &contour);

    void visit_poly_path(const Clipper2Lib::PolyPathD &path, const DocumentRef &docref);

    TopoDS_Compound m_compound;
    TopoDS_Builder m_builder;
    std::list<TopoDS_Wire> m_wires;
    unsigned int m_n_faces = 0;
    bool m_has_hole = false;
    TopoDS_Wire path_to_wire(const Clipper2Lib::PathD &path, bool hole, const DocumentRef &docref);
};
} // namespace solid_model_util

using FaceBuilder = solid_model_util::FaceBuilder;

} // namespace dune3d

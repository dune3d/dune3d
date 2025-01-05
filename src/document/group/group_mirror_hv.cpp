#include "group_mirror_hv.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/document.hpp"
#include "document/solid_model.hpp"
#include "document/entity/entity_arc3d.hpp"
#include "document/entity/entity_circle3d.hpp"
#include "util/glm_util.hpp"
#include <iostream>

namespace dune3d {
glm::dvec2 GroupMirrorHV::transform(const glm::dvec2 &p, unsigned int instance) const
{
    if (instance == 1)
        return p;
    auto m = get_vec2_mul();
    return p * m;
}

glm::dvec3 GroupMirrorVertical::get_n_vector() const
{
    return glm::dvec3(1, 0, 0);
}

glm::dvec3 GroupMirrorHorizontal::get_n_vector() const
{
    return glm::dvec3(0, 1, 0);
}

glm::dvec3 GroupMirrorHV::transform(const Document &doc, const glm::dvec3 &p, unsigned int instance) const
{
    if (instance == 1)
        return p;
    if (!m_active_wrkpl)
        return p;

    auto &wrkpl = doc.get_entity<EntityWorkplane>(m_active_wrkpl);
    const auto v = p - wrkpl.m_origin;
    const auto n = glm::rotate(wrkpl.m_normal, get_n_vector());
    const auto dist = glm::dot(n, v);

    return p - 2 * dist * n;
}

static glm::dvec3 mirror(const glm::dvec3 &v, const glm::dvec3 &n)
{
    const auto dist = glm::dot(n, v);
    return v - 2 * dist * n;
}

glm::dquat GroupMirrorHV::transform_normal(const Document &doc, const glm::dquat &q, unsigned int instance) const
{
    if (instance == 1)
        return q;

    const auto u = glm::rotate(q, glm::dvec3(1, 0, 0));
    const auto v = glm::rotate(q, glm::dvec3(0, 1, 0));
    const auto &wrkpl = doc.get_entity<EntityWorkplane>(m_active_wrkpl);
    const auto n = glm::rotate(wrkpl.m_normal, get_n_vector());
    const auto um = mirror(u, n);
    const auto vm = mirror(v, n);
    return quat_from_uv(um, vm);
}

void GroupMirrorHV::pre_solve(Document &doc) const
{
    for (const auto &[uu, it] : doc.m_entities) {
        if (it->m_group != m_source_group)
            continue;
        if (it->m_construction)
            continue;
        for (unsigned int instance = 0; instance < get_count(); instance++) {
            if (it->of_type(Entity::Type::CIRCLE_3D)) {
                const auto &circle = dynamic_cast<const EntityCircle3D &>(*it);
                auto new_arc_uu = get_entity_uuid(uu, instance);
                if (!doc.m_entities.contains(new_arc_uu))
                    continue;
                auto &new_circle = doc.get_entity<EntityCircle3D>(new_arc_uu);
                new_circle.m_normal = transform_normal(doc, circle.m_normal, instance);
            }
            else if (it->of_type(Entity::Type::ARC_3D)) {
                const auto &arc = dynamic_cast<const EntityArc3D &>(*it);
                auto new_arc_uu = get_entity_uuid(uu, instance);
                if (!doc.m_entities.contains(new_arc_uu))
                    continue;
                auto &new_arc = doc.get_entity<EntityArc3D>(new_arc_uu);
                new_arc.m_normal = transform_normal(doc, arc.m_normal, instance);
            }
        }
    }
}

void GroupMirrorVertical::update_solid_model(const Document &doc)
{
    m_solid_model = SolidModel::create(doc, *this);
}

glm::dvec2 GroupMirrorHorizontal::get_vec2_mul() const
{
    return {1, -1};
}

glm::dvec2 GroupMirrorVertical::get_vec2_mul() const
{
    return {-1, 1};
}

void GroupMirrorHorizontal::update_solid_model(const Document &doc)
{
    m_solid_model = SolidModel::create(doc, *this);
}

std::unique_ptr<Group> GroupMirrorHorizontal::clone() const
{
    return std::make_unique<GroupMirrorHorizontal>(*this);
}

std::unique_ptr<Group> GroupMirrorVertical::clone() const
{
    return std::make_unique<GroupMirrorVertical>(*this);
}

} // namespace dune3d

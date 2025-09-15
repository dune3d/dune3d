#include "group_lathe.hpp"
#include "util/glm_util.hpp"
#include "util/util.hpp"
#include "util/template_util.hpp"
#include "document/document.hpp"
#include "document/entity/entity_circle3d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/entity/ientity_in_workplane.hpp"
#include "document/solid_model/solid_model.hpp"

namespace dune3d {

std::unique_ptr<Group> GroupLathe::clone() const
{
    return std::make_unique<GroupLathe>(*this);
}

UUID GroupLathe::get_lathe_circle_uuid(const UUID &uu, unsigned int pt) const
{
    return hash_uuids("e47e6775-3fe4-473a-84f8-c2c5578a10af", {m_uuid, m_wrkpl, uu},
                      {reinterpret_cast<const uint8_t *>(&pt), sizeof(pt)});
}

static glm::dvec3 project_point_onto_line(const glm::dvec3 &pt, const glm::dvec3 &origin, const glm::dvec3 &dir)
{
    const auto dvec = glm::normalize(dir);
    auto delta = pt - origin;
    auto proj = glm::dot(delta, dvec);
    return origin + proj * dvec;
}

void GroupLathe::pre_solve(Document &doc) const
{
    generate_or_solve(doc, GenerateOrSolve::SOLVE);
}

void GroupLathe::generate(Document &doc)
{
    generate_or_solve(doc, GenerateOrSolve::GENERATE);
}

void GroupLathe::generate_or_solve(Document &doc, GenerateOrSolve gen_or_solve) const
{
    auto &wrkpl = doc.get_entity<EntityWorkplane>(m_wrkpl);
    const auto n = get_direction(doc).value();
    const auto origin = doc.get_point(m_origin);

    for (const auto &[uu, it] : doc.m_entities) {
        if (it->m_group != m_source_group)
            continue;
        if (it->m_construction)
            continue;
        if (any_of(it->get_type(), Entity::Type::LINE_2D, Entity::Type::ARC_2D, Entity::Type::CIRCLE_2D)) {
            const auto &li = dynamic_cast<const IEntityInWorkplane &>(*it);
            if (li.get_workplane() != m_wrkpl)
                continue;
            const unsigned int pt_max = it->get_type() == Entity::Type::CIRCLE_2D ? 1 : 2;
            for (unsigned int pt = 1; pt <= pt_max; pt++) {
                auto new_circle_uu = get_lathe_circle_uuid(uu, pt);
                if (gen_or_solve == GenerateOrSolve::SOLVE && !doc.m_entities.contains(new_circle_uu))
                    continue;
                auto &new_circle = gen_or_solve == GenerateOrSolve::SOLVE
                                           ? doc.get_entity<EntityCircle3D>(new_circle_uu)
                                           : doc.get_or_add_entity<EntityCircle3D>(new_circle_uu);
                new_circle.m_normal = quat_from_uv(wrkpl.get_normal_vector(), glm::cross(wrkpl.get_normal_vector(), n));
                const auto pc = it->get_point(pt, doc);
                new_circle.m_center = project_point_onto_line(pc, origin, n);
                new_circle.m_radius = glm::length(new_circle.m_center - pc);

                if (gen_or_solve == GenerateOrSolve::GENERATE) {
                    new_circle.m_group = m_uuid;
                    new_circle.m_kind = ItemKind::GENRERATED;
                    new_circle.m_generated_from = uu;
                }
            }
        }
    }
}

void GroupLathe::update_solid_model(const Document &doc)
{
    m_solid_model = SolidModel::create(doc, *this);
}

} // namespace dune3d

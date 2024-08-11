#include "constraint_point_distance_aligned.hpp"
#include "constraint_util.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_workplane.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintPointDistanceAligned::ConstraintPointDistanceAligned(const UUID &uu) : ConstraintPointDistanceBase(uu)
{
}

ConstraintPointDistanceAligned::ConstraintPointDistanceAligned(const UUID &uu, const json &j)
    : ConstraintPointDistanceBase(uu, j), m_align_entity(j.at("align_entity").get<UUID>())
{
}

json ConstraintPointDistanceAligned::serialize() const
{
    auto j = ConstraintPointDistanceBase::serialize();
    j["align_entity"] = m_align_entity;
    return j;
}

std::set<EntityAndPoint> ConstraintPointDistanceAligned::get_referenced_entities_and_points() const
{
    auto enps = ConstraintPointDistanceBase::get_referenced_entities_and_points();
    enps.emplace(m_align_entity, 0);
    return enps;
}

bool ConstraintPointDistanceAligned::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    const auto a = ConstraintPointDistanceBase::replace_point(old_point, new_point);
    const auto b = replace_points(old_point, new_point, m_align_entity);
    return a || b;
}

std::unique_ptr<Constraint> ConstraintPointDistanceAligned::clone() const
{
    return std::make_unique<ConstraintPointDistanceAligned>(*this);
}

void ConstraintPointDistanceAligned::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

glm::dvec3 ConstraintPointDistanceAligned::get_align_vector(const Document &doc) const
{
    auto &en = doc.get_entity(m_align_entity);
    glm::dvec3 v;

    if (en.of_type(Entity::Type::LINE_2D, Entity::Type::LINE_3D))
        v = en.get_point(2, doc) - en.get_point(1, doc);
    else if (auto en_wrkpl = dynamic_cast<const EntityWorkplane *>(&en))
        v = en_wrkpl->get_normal_vector();
    else
        throw std::runtime_error("unexpected align entity");

    if (m_wrkpl) {
        auto &wrkpl = doc.get_entity<EntityWorkplane>(m_wrkpl);
        v = wrkpl.project3(v) - wrkpl.project3({0, 0, 0});
    }

    return glm::normalize(v);
}

double ConstraintPointDistanceAligned::measure_distance(const Document &doc) const
{
    return glm::dot(get_distance_vector(doc), get_align_vector(doc));
}

} // namespace dune3d

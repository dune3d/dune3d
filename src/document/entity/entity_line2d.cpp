#include "entity_line2d.hpp"
#include "nlohmann/json.hpp"
#include "util/glm_util.hpp"
#include "util/json_util.hpp"
#include "util/template_util.hpp"
#include "document/document.hpp"
#include "entity_workplane.hpp"
#include "entityt_impl.hpp"
#include "util/bbox_accumulator.hpp"

namespace dune3d {
EntityLine2D::EntityLine2D(const UUID &uu) : Base(uu)
{
}

EntityLine2D::EntityLine2D(const UUID &uu, const json &j)
    : Base(uu, j), m_p1(j.at("p1").get<glm::dvec2>()), m_p2(j.at("p2").get<glm::dvec2>()),
      m_wrkpl(j.at("wrkpl").get<UUID>())
{
}


json EntityLine2D::serialize() const
{
    json j = Entity::serialize();
    j["p1"] = m_p1;
    j["p2"] = m_p2;
    j["wrkpl"] = m_wrkpl;
    return j;
}

double EntityLine2D::get_param(unsigned int point, unsigned int axis) const
{
    if (point == 1) {
        return m_p1[axis];
    }
    else if (point == 2) {
        return m_p2[axis];
    }
    return NAN;
}

void EntityLine2D::set_param(unsigned int point, unsigned int axis, double value)
{
    if (point == 1) {
        m_p1[axis] = value;
    }
    else if (point == 2) {
        m_p2[axis] = value;
    }
}

std::string EntityLine2D::get_point_name(unsigned int point) const
{
    switch (point) {
    case 1:
        return "from";
    case 2:
        return "to";
    default:
        return "";
    }
}

glm::dvec2 EntityLine2D::get_point_in_workplane(unsigned int point) const
{
    if (point == 1)
        return m_p1;
    else if (point == 2)
        return m_p2;
    else
        return {NAN, NAN};
}

glm::dvec3 EntityLine2D::get_point(unsigned int point, const Document &doc) const
{
    auto &wrkpl = doc.get_entity<EntityWorkplane>(m_wrkpl);
    return wrkpl.transform(get_point_in_workplane(point));
}

bool EntityLine2D::is_valid_point(unsigned int point) const
{
    return point == 1 || point == 2;
}

glm::dvec2 EntityLine2D::get_tangent_at_point(unsigned int point) const
{
    if (point == 1)
        return m_p1 - m_p2;
    else
        return m_p2 - m_p1;
}

bool EntityLine2D::is_valid_tangent_point(unsigned int point) const
{
    return any_of(point, 1u, 2u);
}

std::set<UUID> EntityLine2D::get_referenced_entities() const
{
    auto ents = Entity::get_referenced_entities();
    ents.insert(m_wrkpl);
    return ents;
}

void EntityLine2D::move(const Entity &last, const glm::dvec2 &delta, unsigned int point)
{
    auto &en_last = dynamic_cast<const EntityLine2D &>(last);
    if (point == 0 || point == 1) {
        m_p1 = en_last.m_p1 + delta;
    }
    if (point == 0 || point == 2) {
        m_p2 = en_last.m_p2 + delta;
    }
}

std::pair<glm::dvec2, glm::dvec2> EntityLine2D::get_bbox() const
{
    BBoxAccumulator<glm::dvec2> acc;
    acc.accumulate(m_p1);
    acc.accumulate(m_p2);
    return acc.get().value();
}

} // namespace dune3d

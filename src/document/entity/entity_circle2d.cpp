#include "entity_circle2d.hpp"
#include "nlohmann/json.hpp"
#include "util/glm_util.hpp"
#include "util/json_util.hpp"
#include "document/document.hpp"
#include "entity_workplane.hpp"
#include "entityt_impl.hpp"

namespace dune3d {
EntityCircle2D::EntityCircle2D(const UUID &uu) : Base(uu)
{
}

EntityCircle2D::EntityCircle2D(const UUID &uu, const json &j)
    : Base(uu, j), m_center(j.at("center").get<glm::dvec2>()), m_radius(j.at("radius").get<double>()),
      m_wrkpl(j.at("wrkpl").get<UUID>())
{
}

json EntityCircle2D::serialize() const
{
    json j = Entity::serialize();
    j["center"] = m_center;
    j["radius"] = m_radius;
    j["wrkpl"] = m_wrkpl;
    return j;
}

double EntityCircle2D::get_param(unsigned int point, unsigned int axis) const
{
    if (point == 0)
        return m_radius;
    else if (point == 1)
        return m_center[axis];

    return NAN;
}

void EntityCircle2D::set_param(unsigned int point, unsigned int axis, double value)
{
    if (point == 0)
        m_radius = value;

    else if (point == 1)
        m_center[axis] = value;
}

std::string EntityCircle2D::get_point_name(unsigned int point) const
{
    switch (point) {
    case 1:
        return "center";
    default:
        return "";
    }
}

glm::dvec2 EntityCircle2D::get_point_in_workplane(unsigned int point) const
{
    if (point == 1)
        return m_center;
    return {NAN, NAN};
}

glm::dvec3 EntityCircle2D::get_point(unsigned int point, const Document &doc) const
{
    auto &wrkpl = doc.get_entity<EntityWorkplane>(m_wrkpl);
    return wrkpl.transform(get_point_in_workplane(point));
}

bool EntityCircle2D::is_valid_point(unsigned int point) const
{
    return point == 1;
}

std::set<UUID> EntityCircle2D::get_referenced_entities() const
{
    auto ents = Entity::get_referenced_entities();
    ents.insert(m_wrkpl);
    return ents;
}

void EntityCircle2D::move(const Entity &last, const glm::dvec2 &initial, const glm::dvec2 &pos, unsigned int point)
{
    auto &en_last = dynamic_cast<const EntityCircle2D &>(last);
    if (point == 1) {
        m_center = en_last.m_center + (pos - initial);
    }
    else if (point == 0) {
        const auto initial_radius = glm::length(en_last.m_center - initial);
        const auto current_radius = glm::length(m_center - pos);

        m_radius = en_last.m_radius + (current_radius - initial_radius);
    }
}

std::pair<glm::dvec2, glm::dvec2> EntityCircle2D::get_bbox() const
{
    const auto r = glm::dvec2(m_radius, m_radius);
    return {m_center - r, m_center + r};
}

} // namespace dune3d

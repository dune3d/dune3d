#include "entity_arc2d.hpp"
#include "nlohmann/json.hpp"
#include "util/glm_util.hpp"
#include "util/json_util.hpp"
#include "util/template_util.hpp"
#include "document/document.hpp"
#include "entity_workplane.hpp"
#include "entity_visitor.hpp"

namespace dune3d {
EntityArc2D::EntityArc2D(const UUID &uu) : Entity(uu)
{
}

EntityArc2D::EntityArc2D(const UUID &uu, const json &j)
    : Entity(uu, j), m_from(j.at("from").get<glm::dvec2>()), m_to(j.at("to").get<glm::dvec2>()),
      m_center(j.at("center").get<glm::dvec2>()), m_wrkpl(j.at("wrkpl").get<UUID>())
{
}

json EntityArc2D::serialize() const
{
    json j = Entity::serialize();
    j["from"] = m_from;
    j["to"] = m_to;
    j["center"] = m_center;
    j["wrkpl"] = m_wrkpl;
    return j;
}

std::string EntityArc2D::get_point_name(unsigned int point) const
{
    switch (point) {
    case 1:
        return "from";
    case 2:
        return "to";
    case 3:
        return "center";
    default:
        return "";
    }
}

double EntityArc2D::get_param(unsigned int point, unsigned int axis) const
{
    if (point == 1) {
        return m_from[axis];
    }
    else if (point == 2) {
        return m_to[axis];
    }
    else if (point == 3) {
        return m_center[axis];
    }
    return NAN;
}

void EntityArc2D::set_param(unsigned int point, unsigned int axis, double value)
{
    if (point == 1) {
        m_from[axis] = value;
    }
    else if (point == 2) {
        m_to[axis] = value;
    }
    else if (point == 3) {
        m_center[axis] = value;
    }
}


glm::dvec2 EntityArc2D::get_point_in_workplane(unsigned int point) const
{
    if (point == 1)
        return m_from;
    else if (point == 2)
        return m_to;
    else if (point == 3)
        return m_center;
    return {NAN, NAN};
}

glm::dvec3 EntityArc2D::get_point(unsigned int point, const Document &doc) const
{
    auto &wrkpl = doc.get_entity<EntityWorkplane>(m_wrkpl);
    return wrkpl.transform(get_point_in_workplane(point));
}

glm::dvec2 EntityArc2D::get_tangent_at_point(unsigned int point) const
{
    const auto r = ((point == 1) ? m_from : m_to) - m_center;
    if (point == 1)
        return {r.y, -r.x};
    else
        return {-r.y, r.x};
}

bool EntityArc2D::is_valid_tangent_point(unsigned int point) const
{
    return any_of(point, 1, 2);
}

double EntityArc2D::get_radius() const
{
    return glm::length(m_from - m_center);
}

bool EntityArc2D::is_valid_point(unsigned int point) const
{
    return point == 1 || point == 2 || point == 3;
}

std::unique_ptr<Entity> EntityArc2D::clone() const
{
    return std::make_unique<EntityArc2D>(*this);
}

std::set<UUID> EntityArc2D::get_referenced_entities() const
{
    auto ents = Entity::get_referenced_entities();
    ents.insert(m_wrkpl);
    return ents;
}

void EntityArc2D::accept(EntityVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d

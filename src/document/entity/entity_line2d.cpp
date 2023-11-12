#include "entity_line2d.hpp"
#include "nlohmann/json.hpp"
#include "util/glm_util.hpp"
#include "util/json_util.hpp"
#include "document/document.hpp"
#include "entity_workplane.hpp"
#include "entity_visitor.hpp"

namespace dune3d {
EntityLine2D::EntityLine2D(const UUID &uu) : Entity(uu)
{
}

EntityLine2D::EntityLine2D(const UUID &uu, const json &j)
    : Entity(uu, j), m_p1(j.at("p1").get<glm::dvec2>()), m_p2(j.at("p2").get<glm::dvec2>()),
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

std::unique_ptr<Entity> EntityLine2D::clone() const
{
    return std::make_unique<EntityLine2D>(*this);
}

std::set<UUID> EntityLine2D::get_referenced_entities() const
{
    return {m_wrkpl};
}

void EntityLine2D::accept(EntityVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d

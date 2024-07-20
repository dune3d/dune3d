#include "entity_bezier2d.hpp"
#include "nlohmann/json.hpp"
#include "util/glm_util.hpp"
#include "util/json_util.hpp"
#include "util/template_util.hpp"
#include "document/document.hpp"
#include "entity_workplane.hpp"
#include "entity_visitor.hpp"

namespace dune3d {
EntityBezier2D::EntityBezier2D(const UUID &uu) : Entity(uu)
{
}

EntityBezier2D::EntityBezier2D(const UUID &uu, const json &j)
    : Entity(uu, j), m_p1(j.at("p1").get<glm::dvec2>()), m_p2(j.at("p2").get<glm::dvec2>()),
      m_c1(j.at("c1").get<glm::dvec2>()), m_c2(j.at("c2").get<glm::dvec2>()), m_wrkpl(j.at("wrkpl").get<UUID>())
{
}


json EntityBezier2D::serialize() const
{
    json j = Entity::serialize();
    j["p1"] = m_p1;
    j["p2"] = m_p2;
    j["c1"] = m_c1;
    j["c2"] = m_c2;
    j["wrkpl"] = m_wrkpl;
    return j;
}

double EntityBezier2D::get_param(unsigned int point, unsigned int axis) const
{
    if (point == 1) {
        return m_p1[axis];
    }
    else if (point == 2) {
        return m_p2[axis];
    }
    else if (point == 3) {
        return m_c1[axis];
    }
    else if (point == 4) {
        return m_c2[axis];
    }
    return NAN;
}

void EntityBezier2D::set_param(unsigned int point, unsigned int axis, double value)
{
    if (point == 1) {
        m_p1[axis] = value;
    }
    else if (point == 2) {
        m_p2[axis] = value;
    }
    else if (point == 3) {
        m_c1[axis] = value;
    }
    else if (point == 4) {
        m_c2[axis] = value;
    }
}

std::string EntityBezier2D::get_point_name(unsigned int point) const
{
    switch (point) {
    case 1:
        return "from";
    case 2:
        return "to";
    case 3:
        return "from handle";
    case 4:
        return "to handle";
    default:
        return "";
    }
}

glm::dvec2 EntityBezier2D::get_interpolated(double t) const
{
    return m_p1 * pow(1 - t, 3) + 3. * m_c1 * pow(1 - t, 2) * t + 3. * m_c2 * (1 - t) * pow(t, 2) + m_p2 * pow(t, 3);
}

glm::dvec2 EntityBezier2D::get_point_in_workplane(unsigned int point) const
{
    if (point == 1)
        return m_p1;
    else if (point == 2)
        return m_p2;
    else if (point == 3)
        return m_c1;
    else if (point == 4)
        return m_c2;
    else
        return {NAN, NAN};
}

glm::dvec3 EntityBezier2D::get_point(unsigned int point, const Document &doc) const
{
    auto &wrkpl = doc.get_entity<EntityWorkplane>(m_wrkpl);
    return wrkpl.transform(get_point_in_workplane(point));
}

bool EntityBezier2D::is_valid_point(unsigned int point) const
{
    return point == 1 || point == 2 || point == 3 || point == 4;
}

glm::dvec2 EntityBezier2D::get_tangent_at_point(unsigned int point) const
{
    if (point == 1)
        return m_p1 - m_c1;
    else
        return m_p2 - m_c2;
}

bool EntityBezier2D::is_valid_tangent_point(unsigned int point) const
{
    return any_of(point, 1, 2);
}

std::unique_ptr<Entity> EntityBezier2D::clone() const
{
    return std::make_unique<EntityBezier2D>(*this);
}

std::set<UUID> EntityBezier2D::get_referenced_entities() const
{
    auto ents = Entity::get_referenced_entities();
    ents.insert(m_wrkpl);
    return ents;
}

void EntityBezier2D::accept(EntityVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d

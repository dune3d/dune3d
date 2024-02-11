#include "entity_circle2d.hpp"
#include "nlohmann/json.hpp"
#include "util/glm_util.hpp"
#include "util/json_util.hpp"
#include "document/document.hpp"
#include "entity_workplane.hpp"
#include "entity_visitor.hpp"

namespace dune3d {
EntityCircle2D::EntityCircle2D(const UUID &uu) : Entity(uu)
{
}

EntityCircle2D::EntityCircle2D(const UUID &uu, const json &j)
    : Entity(uu, j), m_center(j.at("center").get<glm::dvec2>()), m_radius(j.at("radius").get<double>()),
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

std::unique_ptr<Entity> EntityCircle2D::clone() const
{
    return std::make_unique<EntityCircle2D>(*this);
}

std::set<UUID> EntityCircle2D::get_referenced_entities() const
{
    auto ents = Entity::get_referenced_entities();
    ents.insert(m_wrkpl);
    return ents;
}

void EntityCircle2D::accept(EntityVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d

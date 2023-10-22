#include "entity_circle3d.hpp"
#include "nlohmann/json.hpp"
#include "util/glm_util.hpp"
#include "util/json_util.hpp"
#include "document.hpp"
#include "entity_workplane.hpp"
#include "entity_visitor.hpp"

namespace dune3d {
EntityCircle3D::EntityCircle3D(const UUID &uu) : Entity(uu)
{
}

EntityCircle3D::EntityCircle3D(const UUID &uu, const json &j)
    : Entity(uu, j), m_center(j.at("center").get<glm::dvec3>()), m_radius(j.at("radius").get<double>()),
      m_normal(j.at("normal").get<glm::dquat>())
{
}

json EntityCircle3D::serialize() const
{
    json j = Entity::serialize();
    j["center"] = m_center;
    j["radius"] = m_radius;
    j["normal"] = m_normal;
    return j;
}

double EntityCircle3D::get_param(unsigned int point, unsigned int axis) const
{
    if (point == 0)
        return m_radius;
    else if (point == 1)
        return m_center[axis];
    else if (point == 2)
        return m_normal[axis];

    return NAN;
}

void EntityCircle3D::set_param(unsigned int point, unsigned int axis, double value)
{
    if (point == 0)
        m_radius = value;
    else if (point == 1)
        m_center[axis] = value;
    else if (point == 2)
        m_normal[axis] = value;
}

glm::dvec3 EntityCircle3D::get_point(unsigned int point, const Document &doc) const
{
    if (point == 1)
        return m_center;
    return {NAN, NAN, NAN};
}

bool EntityCircle3D::is_valid_point(unsigned int point) const
{
    return point == 1;
}

std::unique_ptr<Entity> EntityCircle3D::clone() const
{
    return std::make_unique<EntityCircle3D>(*this);
}

void EntityCircle3D::accept(EntityVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d

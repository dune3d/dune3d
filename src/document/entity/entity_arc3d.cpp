#include "entity_arc3d.hpp"
#include "nlohmann/json.hpp"
#include "util/glm_util.hpp"
#include "util/json_util.hpp"
#include "document/document.hpp"
#include "entity_visitor.hpp"

namespace dune3d {
EntityArc3D::EntityArc3D(const UUID &uu) : Entity(uu)
{
}

json EntityArc3D::serialize() const
{
    throw std::runtime_error("not implemented");
    return nullptr;
}

double EntityArc3D::get_param(unsigned int point, unsigned int axis) const
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
    else if (point == 4) {
        return m_normal[axis];
    }
    return NAN;
}

void EntityArc3D::set_param(unsigned int point, unsigned int axis, double value)
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
    else if (point == 4) {
        m_normal[axis] = value;
    }
}

glm::dvec3 EntityArc3D::get_point(unsigned int point, const Document &doc) const
{
    if (point == 1)
        return m_from;
    else if (point == 2)
        return m_to;
    else if (point == 3)
        return m_center;
    return {NAN, NAN, NAN};
}

bool EntityArc3D::is_valid_point(unsigned int point) const
{
    return point == 1 || point == 2 || point == 3;
}

std::unique_ptr<Entity> EntityArc3D::clone() const
{
    return std::make_unique<EntityArc3D>(*this);
}

void EntityArc3D::accept(EntityVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d

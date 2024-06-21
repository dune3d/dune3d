#include "entity_workplane.hpp"
#include "nlohmann/json.hpp"
#include "util/glm_util.hpp"
#include "entity_visitor.hpp"

namespace dune3d {
EntityWorkplane::EntityWorkplane(const UUID &uu)
    : Entity(uu), m_normal(glm::quat_identity<double, glm::defaultp>()), m_size(10, 10)
{
}

EntityWorkplane::EntityWorkplane(const UUID &uu, const json &j)
    : Entity(uu, j), m_origin(j.at("origin").get<glm::dvec3>()), m_normal(j.at("normal").get<glm::dquat>()),
      m_size(j.at("size").get<glm::dvec2>())
{
}


json EntityWorkplane::serialize() const
{
    json j = Entity::serialize();
    j["origin"] = m_origin;
    j["normal"] = m_normal;
    j["size"] = m_size;
    return j;
}

double EntityWorkplane::get_param(unsigned int point, unsigned int axis) const
{
    if (point == 1) {
        return m_origin[axis];
    }
    else if (point == 2) {
        return m_normal[axis];
    }
    return NAN;
}

void EntityWorkplane::set_param(unsigned int point, unsigned int axis, double value)
{
    if (point == 1) {
        m_origin[axis] = value;
    }
    else if (point == 2) {
        m_normal[axis] = value;
    }
}

std::unique_ptr<Entity> EntityWorkplane::clone() const
{
    return std::make_unique<EntityWorkplane>(*this);
}

glm::dvec3 EntityWorkplane::get_normal_vector() const
{
    return glm::rotate(m_normal, glm::dvec3(0, 0, 1));
}

glm::dvec3 EntityWorkplane::transform_relative(glm::dvec2 p) const
{
    return glm::rotate(m_normal, glm::dvec3(p, 0));
}

glm::dvec3 EntityWorkplane::transform(glm::dvec2 p) const
{
    return m_origin + transform_relative(p);
}

glm::dvec2 EntityWorkplane::project(glm::dvec3 p) const
{
    const auto v = p - m_origin;
    auto un = glm::rotate(m_normal, glm::dvec3(1, 0, 0));
    auto vn = glm::rotate(m_normal, glm::dvec3(0, 1, 0));
    return {glm::dot(un, v), glm::dot(vn, v)};
}

glm::dvec3 EntityWorkplane::project3(glm::dvec3 p) const
{
    return transform(project(p));
}

std::string EntityWorkplane::get_point_name(unsigned int point) const
{
    switch (point) {
    case 1:
        return "origin";
    default:
        return "";
    }
}

glm::dvec3 EntityWorkplane::get_point(unsigned int point, const Document &doc) const
{
    if (point == 1)
        return m_origin;
    return {NAN, NAN, NAN};
}

bool EntityWorkplane::is_valid_point(unsigned int point) const
{
    return point == 1;
}

void EntityWorkplane::accept(EntityVisitor &visitor) const
{
    visitor.visit(*this);
}


} // namespace dune3d

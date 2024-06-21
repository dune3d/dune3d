#include "entity_line3d.hpp"
#include "nlohmann/json.hpp"
#include "util/glm_util.hpp"
#include "entity_visitor.hpp"

/*
NLOHMANN_JSON_NAMESPACE_BEGIN
template <> struct adl_serializer<glm::vec3> {
    static void to_json(json &j, const glm::vec3 &v)
    {
        j = json::array({v.x, v.y, v.z});
    }

    static void from_json(const json &j, glm::vec3 &v)
    {
    }
};
NLOHMANN_JSON_NAMESPACE_END
*/
/*

namespace nlohmann {

} // namespace nlohmann
*/


// namespace glm

namespace dune3d {
EntityLine3D::EntityLine3D(const UUID &uu) : Entity(uu)
{
}

EntityLine3D::EntityLine3D(const UUID &uu, const json &j)
    : Entity(uu, j), m_p1(j.at("p1").get<glm::dvec3>()), m_p2(j.at("p2").get<glm::dvec3>())
{
}


json EntityLine3D::serialize() const
{
    json j = Entity::serialize();
    j["p1"] = m_p1;
    j["p2"] = m_p2;
    return j;
}

double EntityLine3D::get_param(unsigned int point, unsigned int axis) const
{
    if (point == 1) {
        return m_p1[axis];
    }
    else if (point == 2) {
        return m_p2[axis];
    }
    return NAN;
}

void EntityLine3D::set_param(unsigned int point, unsigned int axis, double value)
{
    if (point == 1) {
        m_p1[axis] = value;
    }
    else if (point == 2) {
        m_p2[axis] = value;
    }
}

std::string EntityLine3D::get_point_name(unsigned int point) const
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

glm::dvec3 EntityLine3D::get_point(unsigned int point, const Document &doc) const
{
    if (point == 1)
        return m_p1;
    else if (point == 2)
        return m_p2;
    return {NAN, NAN, NAN};
}

bool EntityLine3D::is_valid_point(unsigned int point) const
{
    return point == 1 || point == 2;
}


std::unique_ptr<Entity> EntityLine3D::clone() const
{
    return std::make_unique<EntityLine3D>(*this);
}

void EntityLine3D::accept(EntityVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d

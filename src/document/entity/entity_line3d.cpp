#include "entity_line3d.hpp"
#include "nlohmann/json.hpp"
#include "util/glm_util.hpp"
#include "entityt_impl.hpp"

namespace dune3d {
EntityLine3D::EntityLine3D(const UUID &uu) : Base(uu)
{
}

EntityLine3D::EntityLine3D(const UUID &uu, const json &j)
    : Base(uu, j), m_p1(j.at("p1").get<glm::dvec3>()), m_p2(j.at("p2").get<glm::dvec3>())
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


} // namespace dune3d

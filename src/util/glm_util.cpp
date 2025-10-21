#include "glm_util.hpp"
#include "nlohmann/json.hpp"
#include <glm/gtx/quaternion.hpp>

namespace glm {

void to_json(nlohmann::json &j, const dvec3 &v)
{
    j = nlohmann::json::array({v.x, v.y, v.z});
}

void from_json(const nlohmann::json &j, dvec3 &v)
{
    j.at(0).get_to(v.x);
    j.at(1).get_to(v.y);
    j.at(2).get_to(v.z);
}

void to_json(nlohmann::json &j, const dvec2 &v)
{
    j = nlohmann::json::array({v.x, v.y});
}

void from_json(const nlohmann::json &j, dvec2 &v)
{
    j.at(0).get_to(v.x);
    j.at(1).get_to(v.y);
}

void to_json(nlohmann::json &j, const dquat &q)
{
    j = nlohmann::json::array({q.x, q.y, q.z, q.w});
}

void from_json(const nlohmann::json &j, dquat &q)
{
    j.at(0).get_to(q.x);
    j.at(1).get_to(q.y);
    j.at(2).get_to(q.z);
    j.at(3).get_to(q.w);
}

} // namespace glm

namespace dune3d {

glm::dvec2 project_onto_perp_bisector(const glm::dvec2 &a, const glm::dvec2 &b, const glm::dvec2 &p)
{
    const auto c = (a + b) / 2.;
    const auto d = b - a;
    if (d.length() == 0)
        return p;
    const auto u = (glm::dot(d, c) - glm::dot(d, p)) / (glm::length(d) * glm::length(d));
    return p + d * u;
}

glm::dquat quat_from_uv(const glm::dvec3 &u, const glm::dvec3 &v)
{

    auto n = glm::cross(u, v);
    glm::dquat q;
    double s, tr = 1 + u.x + v.y + n.z;
    if (tr > 1e-4) {
        s = 2 * sqrt(tr);
        q.w = s / 4;
        q.x = (v.z - n.y) / s;
        q.y = (n.x - u.z) / s;
        q.z = (u.y - v.x) / s;
    }
    else {
        if (u.x > v.y && u.x > n.z) {
            s = 2 * sqrt(1 + u.x - v.y - n.z);
            q.w = (v.z - n.y) / s;
            q.x = s / 4;
            q.y = (u.y + v.x) / s;
            q.z = (n.x + u.z) / s;
        }
        else if (v.y > n.z) {
            s = 2 * sqrt(1 - u.x + v.y - n.z);
            q.w = (n.x - u.z) / s;
            q.x = (u.y + v.x) / s;
            q.y = s / 4;
            q.z = (v.z + n.y) / s;
        }
        else {
            s = 2 * sqrt(1 - u.x - v.y + n.z);
            q.w = (u.y - v.x) / s;
            q.x = (n.x + u.z) / s;
            q.y = (v.z + n.y) / s;
            q.z = s / 4;
        }
    }

    return glm::normalize(q);
}


unsigned int abs_max_axis(const glm::dvec3 &v)
{
    auto a = glm::abs(v);
    if (a.x > a.y && a.x > a.z)
        return 0;
    else if (a.y > a.x && a.y > a.z)
        return 1;
    else
        return 2;
}

static std::string axis_from_vec(const glm::dvec3 &v)
{
    const auto ax = abs_max_axis(v);
    static const std::string xyz = "XYZ";

    return (v[ax] > 0 ? "+" : "−") + std::string(1, xyz[ax]);
}

std::string quat_to_string(const glm::dquat &q)
{
    const glm::dvec3 u = glm::rotate(q, glm::dvec3(1, 0, 0));
    const glm::dvec3 v = glm::rotate(q, glm::dvec3(0, 1, 0));
    const glm::dvec3 n = glm::rotate(q, glm::dvec3(0, 0, 1));

    return "U:<tt>" + axis_from_vec(u) + "</tt> V:<tt>" + axis_from_vec(v) + "</tt> N:<tt>" + axis_from_vec(n)
           + "</tt>";
}

} // namespace dune3d

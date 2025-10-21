#pragma once
#include <glm/glm.hpp>
#include <nlohmann/json_fwd.hpp>

namespace glm {
void to_json(nlohmann::json &j, const dvec3 &v);
void from_json(const nlohmann::json &j, dvec3 &v);

void to_json(nlohmann::json &j, const dvec2 &v);
void from_json(const nlohmann::json &j, dvec2 &v);

void to_json(nlohmann::json &j, const dquat &v);
void from_json(const nlohmann::json &j, dquat &v);
} // namespace glm

namespace dune3d {

glm::dvec2 project_onto_perp_bisector(const glm::dvec2 &a, const glm::dvec2 &b, const glm::dvec2 &p);
glm::dquat quat_from_uv(const glm::dvec3 &u, const glm::dvec3 &v);

unsigned int abs_max_axis(const glm::dvec3 &v);
std::string quat_to_string(const glm::dquat &q);

// template <typename T> double get_axis(T p, unsigned int axis);
// template <typename T> void set_axis(T &p, unsigned int axis, double value);
} // namespace dune3d

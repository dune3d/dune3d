#include "entity_document.hpp"
#include "nlohmann/json.hpp"
#include "util/glm_util.hpp"
#include "entityt_impl.hpp"
#include <format>

namespace dune3d {
EntityDocument::EntityDocument(const UUID &uu) : Base(uu), m_normal(glm::quat_identity<double, glm::defaultp>())
{
}

EntityDocument::EntityDocument(const UUID &uu, const json &j)
    : Base(uu, j), m_origin(j.at("origin").get<glm::dvec3>()), m_normal(j.at("normal").get<glm::dquat>()),
      m_path(j.at("path").get<std::filesystem::path>())
{
}

std::filesystem::path EntityDocument::get_path(const std::filesystem::path &containing_dir) const
{
    if (m_path.is_absolute())
        return m_path;
    else
        return containing_dir / m_path;
}

json EntityDocument::serialize() const
{
    json j = Entity::serialize();
    j["origin"] = m_origin;
    j["normal"] = m_normal;
    j["path"] = m_path;
    return j;
}


glm::dvec3 EntityDocument::transform(glm::dvec3 p) const
{
    return glm::rotate(m_normal, p) + m_origin;
}

double EntityDocument::get_param(unsigned int point, unsigned int axis) const
{
    if (point == 1) {
        return m_origin[axis];
    }
    else if (point == 2) {
        return m_normal[axis];
    }
    else if (m_anchors_transformed.contains(point)) {
        return m_anchors_transformed.at(point)[axis];
    }
    return NAN;
}

void EntityDocument::set_param(unsigned int point, unsigned int axis, double value)
{
    if (point == 1) {
        m_origin[axis] = value;
    }
    else if (point == 2) {
        m_normal[axis] = value;
    }
    else if (m_anchors_transformed.contains(point)) {
        m_anchors_transformed.at(point)[axis] = value;
    }
}

glm::dvec3 EntityDocument::get_point(unsigned int point, const Document &doc) const
{
    if (point == 1)
        return m_origin;
    if (m_anchors_transformed.contains(point))
        return m_anchors_transformed.at(point);
    return {NAN, NAN, NAN};
}

std::string EntityDocument::get_point_name(unsigned int point) const
{
    switch (point) {
    case 0:
        return "";
    case 1:
        return "origin";
    default:
        if (m_anchors.contains(point))
            return std::format("anchor {}", point);
        else
            return "";
    }
}

bool EntityDocument::is_valid_point(unsigned int point) const
{
    return point == 1 || m_anchors.contains(point);
}

void EntityDocument::add_anchor(unsigned int i, const glm::dvec3 &pt)
{
    m_anchors.emplace(i, pt);
    m_anchors_transformed.emplace(i, transform(pt));
}

void EntityDocument::update_anchor(unsigned int i, const glm::dvec3 &pt)
{
    m_anchors.at(i) = pt;
    m_anchors_transformed.at(i) = transform(pt);
}

void EntityDocument::remove_anchor(unsigned int i)
{
    m_anchors.erase(i);
    m_anchors_transformed.erase(i);
}

void EntityDocument::move(const Entity &last, const glm::dvec3 &delta, unsigned int point)
{
    auto &en_last = dynamic_cast<const EntityDocument &>(last);
    if (point == 0 || point == 1)
        m_origin = en_last.m_origin + delta;
}

} // namespace dune3d

#include "entity_text.hpp"
#include "nlohmann/json.hpp"
#include "util/glm_util.hpp"
#include "util/json_util.hpp"
#include "document/document.hpp"
#include "entity_workplane.hpp"
#include "entityt_impl.hpp"
#include "document/constraint/constraint.hpp"
#include <glm/gtx/rotate_vector.hpp>
#include <format>

namespace dune3d {
EntityText::EntityText(const UUID &uu) : Base(uu), m_content(ClusterContent::create())
{
}

EntityText::EntityText(const UUID &uu, const json &j)
    : Base(uu, j), m_origin(j.at("origin").get<glm::dvec2>()), m_scale(j.at("scale").get<double>()),
      m_angle(j.at("angle").get<double>()), m_lock_scale(j.value("lock_scale", false)),
      m_lock_angle(j.value("lock_angle", false)), m_text(j.at("text").get<std::string>()),
      m_font(j.at("font").get<std::string>()), m_font_features(j.at("font_features").get<std::string>()),
      m_wrkpl(j.at("wrkpl").get<UUID>())
{
    m_content = ClusterContent::from_json(j);
    if (j.contains("anchors")) {
        for (const auto &[k, v] : j.at("anchors").items()) {
            m_anchors.emplace(std::stoi(k), v.get<glm::dvec2>());
            m_anchors_transformed.emplace(std::stoi(k), transform(v.get<glm::dvec2>()));
        }
    }
}


json EntityText::serialize() const
{
    json j = Entity::serialize();
    j["origin"] = m_origin;
    j["scale"] = m_scale;
    j["lock_scale"] = m_lock_scale;
    j["lock_angle"] = m_lock_angle;
    j["angle"] = m_angle;
    j["wrkpl"] = m_wrkpl;
    j["text"] = m_text;
    j["font"] = m_font;
    j["font_features"] = m_font_features;
    m_content->serialize(j);
    {
        json o = json::object();
        for (const auto &[k, v] : m_anchors) {
            o[std::to_string(k)] = v;
        }
        j["anchors"] = o;
    }
    return j;
}

double EntityText::get_param(unsigned int point, unsigned int axis) const
{
    if (point == 1) {
        return m_origin[axis];
    }
    else if (point == 2) {
        return m_scale;
    }
    else if (point == 3) {
        return glm::radians(m_angle);
    }
    else if (m_anchors_transformed.contains(point)) {
        return m_anchors_transformed.at(point)[axis];
    }
    return NAN;
}

void EntityText::set_param(unsigned int point, unsigned int axis, double value)
{
    if (point == 1) {
        m_origin[axis] = value;
    }
    else if (point == 2) {
        m_scale = value;
    }
    else if (point == 3) {
        m_angle = glm::degrees(value);
    }
    else if (m_anchors_transformed.contains(point)) {
        m_anchors_transformed.at(point)[axis] = value;
    }
}

std::string EntityText::get_point_name(unsigned int point) const
{
    switch (point) {
    case 1:
        return "origin";
    case get_anchor_index(AnchorX::LEFT, AnchorY::ASCEND):
        return "Left ascend";
    case get_anchor_index(AnchorX::LEFT, AnchorY::DESCEND):
        return "Left descend";
    case get_anchor_index(AnchorX::LEFT, AnchorY::BOTTOM):
        return "Left bottom";
    case get_anchor_index(AnchorX::LEFT, AnchorY::TOP):
        return "Left top";
    case get_anchor_index(AnchorX::RIGHT, AnchorY::ASCEND):
        return "Right ascend";
    case get_anchor_index(AnchorX::RIGHT, AnchorY::DESCEND):
        return "Right descend";
    case get_anchor_index(AnchorX::RIGHT, AnchorY::BOTTOM):
        return "Right bottom";
    case get_anchor_index(AnchorX::RIGHT, AnchorY::TOP):
        return "Right top";
    case get_anchor_index(AnchorX::RIGHT, AnchorY::BASE):
        return "Right base";
    default:
        return "";
    }
}

glm::dvec2 EntityText::get_point_in_workplane(unsigned int point) const
{
    if (point == 1)
        return m_origin;
    else if (m_anchors_transformed.contains(point))
        return m_anchors_transformed.at(point);
    else
        return {NAN, NAN};
}

glm::dvec3 EntityText::get_point(unsigned int point, const Document &doc) const
{
    auto &wrkpl = doc.get_entity<EntityWorkplane>(m_wrkpl);
    return wrkpl.transform(get_point_in_workplane(point));
}

bool EntityText::is_valid_point(unsigned int point) const
{
    return point == 1 || m_anchors.contains(point);
}

std::set<UUID> EntityText::get_referenced_entities() const
{
    auto ents = Entity::get_referenced_entities();
    ents.insert(m_wrkpl);
    return ents;
}

glm::dvec2 EntityText::transform(const glm::dvec2 &p) const
{
    return m_origin + glm::rotate(p * glm::dvec2{m_scale, m_scale}, glm::radians(m_angle));
}

void EntityText::move(const Entity &last, const glm::dvec2 &delta, unsigned int point)
{
    auto &en_last = dynamic_cast<const EntityText &>(last);
    if (point == 0 || point == 1)
        m_origin = en_last.m_origin + delta;
    else if (m_anchors_transformed.contains(point) && en_last.m_anchors_transformed.contains(point))
        m_anchors_transformed.at(point) = en_last.m_anchors_transformed.at(point) + delta;
}

void EntityText::add_anchor(unsigned int i, const glm::dvec2 &pt)
{
    m_anchors.emplace(i, pt);
    m_anchors_transformed.emplace(i, transform(pt));
}

void EntityText::clear_anchors()
{
    m_anchors.clear();
    m_anchors_transformed.clear();
}

} // namespace dune3d

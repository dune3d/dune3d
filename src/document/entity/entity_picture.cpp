#include "entity_picture.hpp"
#include "nlohmann/json.hpp"
#include "util/glm_util.hpp"
#include "util/json_util.hpp"
#include "util/bbox_accumulator.hpp"
#include "document/document.hpp"
#include "entity_workplane.hpp"
#include "entityt_impl.hpp"
#include "util/picture_data.hpp"
#include "document/constraint/constraint.hpp"
#include <glm/gtx/rotate_vector.hpp>
#include <format>

namespace dune3d {
EntityPicture::EntityPicture(const UUID &uu) : Base(uu)
{
}

EntityPicture::EntityPicture(const UUID &uu, const json &j)
    : Base(uu, j), m_origin(j.at("origin").get<glm::dvec2>()), m_scale_x(j.at("scale_x").get<double>()),
      m_scale_y(j.at("scale_y").get<double>()), m_angle(j.at("angle").get<double>()),
      m_lock_aspect_ratio(j.value("lock_aspect_ratio", false)), m_lock_angle(j.value("lock_angle", false)),
      m_wrkpl(j.at("wrkpl").get<UUID>()), m_data_uuid(j.at("data").get<UUID>()), m_width(j.at("width").get<double>()),
      m_height(j.at("height").get<double>())
{
    update_builtin_anchors();
    for (const auto &[k, v] : j.at("anchors").items()) {
        m_anchors.emplace(std::stoi(k), v.get<glm::dvec2>());
        m_anchors_transformed.emplace(std::stoi(k), transform(v.get<glm::dvec2>()));
    }
}


json EntityPicture::serialize() const
{
    json j = Entity::serialize();
    j["origin"] = m_origin;
    j["scale_x"] = m_scale_x;
    j["scale_y"] = m_scale_y;
    j["lock_aspect_ratio"] = m_lock_aspect_ratio;
    j["lock_angle"] = m_lock_angle;
    j["angle"] = m_angle;
    j["wrkpl"] = m_wrkpl;
    j["data"] = m_data_uuid;
    j["width"] = m_width;
    j["height"] = m_height;
    {
        json o = json::object();
        for (const auto &[k, v] : m_anchors) {
            o[std::to_string(k)] = v;
        }
        j["anchors"] = o;
    }
    return j;
}

double EntityPicture::get_param(unsigned int point, unsigned int axis) const
{
    if (point == 1) {
        return m_origin[axis];
    }
    else if (point == 2) {
        if (axis == 0)
            return m_scale_x;
        else
            return m_scale_y;
    }
    else if (point == 3) {
        return glm::radians(m_angle);
    }
    else if (m_anchors_transformed.contains(point)) {
        return m_anchors_transformed.at(point)[axis];
    }
    return NAN;
}

void EntityPicture::set_param(unsigned int point, unsigned int axis, double value)
{
    if (point == 1) {
        m_origin[axis] = value;
    }
    else if (point == 2) {
        if (axis == 0)
            m_scale_x = value;
        else
            m_scale_y = value;
    }
    else if (point == 3) {
        m_angle = glm::degrees(value);
    }
    else if (m_anchors_transformed.contains(point)) {
        m_anchors_transformed.at(point)[axis] = value;
    }
}

std::string EntityPicture::get_point_name(unsigned int point) const
{
    switch (point) {
    case 1:
        return "origin";
    case 10:
        return "top left";
    case 11:
        return "bottm left";
    case 12:
        return "top right";
    case 13:
        return "bottom right";
    default:
        if (m_anchors.contains(point)) {
            auto a = m_anchors.at(point);
            return std::format("anchor {} at {:.0f}, {:.0f}", point, a.x, a.y);
        }
        else {
            return "";
        }
    }
}

glm::dvec2 EntityPicture::get_point_in_workplane(unsigned int point) const
{
    if (point == 1)
        return m_origin;
    else if (m_anchors.contains(point))
        return transform(m_anchors.at(point));
    else
        return {NAN, NAN};
}

glm::dvec3 EntityPicture::get_point(unsigned int point, const Document &doc) const
{
    auto &wrkpl = doc.get_entity<EntityWorkplane>(m_wrkpl);
    return wrkpl.transform(get_point_in_workplane(point));
}

bool EntityPicture::is_valid_point(unsigned int point) const
{
    return point == 1 || m_anchors_transformed.contains(point);
}

std::set<UUID> EntityPicture::get_referenced_entities() const
{
    auto ents = Entity::get_referenced_entities();
    ents.insert(m_wrkpl);
    return ents;
}

glm::dvec2 EntityPicture::transform(const glm::dvec2 &p) const
{
    return m_origin
           + glm::rotate((p + glm::dvec2{-m_width, -m_height} / 2.) * glm::dvec2{m_scale_x, m_scale_y},
                         glm::radians(m_angle));
}

glm::dvec2 EntityPicture::untransform(const glm::dvec2 &p) const
{
    const auto translated = p - m_origin;
    const auto rotated = glm::rotate(translated, -glm::radians(m_angle));
    const auto scaled = rotated / glm::dvec2{m_scale_x, m_scale_y};
    return scaled - glm::dvec2{-m_width, -m_height} / 2.;
}


void EntityPicture::add_anchor(unsigned int i, const glm::dvec2 &pt)
{
    m_anchors.emplace(i, pt);
    m_anchors_transformed.emplace(i, transform(pt));
}

static const unsigned int first_user_anchor = 20;

unsigned int EntityPicture::add_anchor(const glm::dvec2 &pt)
{
    unsigned int i = first_user_anchor;
    while (m_anchors.contains(i)) {
        i++;
    }
    add_anchor(i, pt);
    return i;
}

void EntityPicture::update_anchor(unsigned int i, const glm::dvec2 &pt)
{
    m_anchors.at(i) = pt;
    m_anchors_transformed.at(i) = transform(pt);
}

void EntityPicture::remove_anchor(unsigned int i)
{
    m_anchors.erase(i);
    m_anchors_transformed.erase(i);
}

bool EntityPicture::delete_point(unsigned int i)
{
    if (is_user_anchor(i)) {
        remove_anchor(i);
        return true;
    }
    return false;
}

bool EntityPicture::is_user_anchor(unsigned int i) const
{
    return i >= first_user_anchor && m_anchors.contains(i);
}

void EntityPicture::update_builtin_anchors()
{
    remove_anchor(10);
    remove_anchor(11);
    remove_anchor(12);
    remove_anchor(13);

    add_anchor(10, {0, m_height});
    add_anchor(11, {0, 0});
    add_anchor(12, {m_width, m_height});
    add_anchor(13, {m_width, 0});
}


void EntityPicture::move(const Entity &last, const glm::dvec2 &delta, unsigned int point)
{
    auto &en_last = dynamic_cast<const EntityPicture &>(last);
    if (point == 0 || point == 1)
        m_origin = en_last.m_origin + delta;
    else if (m_anchors_transformed.contains(point) && en_last.m_anchors_transformed.contains(point))
        m_anchors_transformed.at(point) = en_last.m_anchors_transformed.at(point) + delta;
}

std::pair<glm::dvec2, glm::dvec2> EntityPicture::get_bbox() const
{
    BBoxAccumulator<glm::dvec2> acc;
    for (unsigned int point = 10; point < 14; point++) {
        acc.accumulate(get_point_in_workplane(point));
    }
    return acc.get().value();
}


} // namespace dune3d

#include "entity_point2d.hpp"
#include "nlohmann/json.hpp"
#include "util/glm_util.hpp"
#include "util/json_util.hpp"
#include "document/document.hpp"
#include "entity_workplane.hpp"
#include "entity_visitor.hpp"
#include "entityt_impl.hpp"

namespace dune3d {
EntityPoint2D::EntityPoint2D(const UUID &uu) : Base(uu)
{
}

EntityPoint2D::EntityPoint2D(const UUID &uu, const json &j)
    : Base(uu, j), m_p(j.at("p").get<glm::dvec2>()), m_wrkpl(j.at("wrkpl").get<UUID>())
{
}

json EntityPoint2D::serialize() const
{
    json j = Entity::serialize();
    j["p"] = m_p;
    j["wrkpl"] = m_wrkpl;
    return j;
}

double EntityPoint2D::get_param(unsigned int point, unsigned int axis) const
{
    if (point == 0) {
        return m_p[axis];
    }
    return NAN;
}

void EntityPoint2D::set_param(unsigned int point, unsigned int axis, double value)
{
    if (point == 0) {
        m_p[axis] = value;
    }
}

glm::dvec2 EntityPoint2D::get_point_in_workplane(unsigned int point) const
{
    if (point == 0)
        return m_p;
    else
        return {NAN, NAN};
}

glm::dvec3 EntityPoint2D::get_point(unsigned int point, const Document &doc) const
{
    auto &wrkpl = doc.get_entity<EntityWorkplane>(m_wrkpl);
    return wrkpl.transform(get_point_in_workplane(point));
}

bool EntityPoint2D::is_valid_point(unsigned int point) const
{
    return point == 0;
}

std::set<UUID> EntityPoint2D::get_referenced_entities() const
{
    auto ents = Entity::get_referenced_entities();
    ents.insert(m_wrkpl);
    return ents;
}

void EntityPoint2D::move(const Entity &last, const glm::dvec2 &delta, unsigned int point)
{
    auto &en_last = dynamic_cast<const EntityPoint2D &>(last);
    if (point == 0) {
        m_p = en_last.m_p + delta;
    }
}

std::pair<glm::dvec2, glm::dvec2> EntityPoint2D::get_bbox() const
{
    return {m_p, m_p};
}

} // namespace dune3d

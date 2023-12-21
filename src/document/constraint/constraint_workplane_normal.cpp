#include "constraint_workplane_normal.hpp"
#include "document/document.hpp"
#include "util/glm_util.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_workplane.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintWorkplaneNormal::ConstraintWorkplaneNormal(const UUID &uu) : Constraint(uu)
{
}

ConstraintWorkplaneNormal::ConstraintWorkplaneNormal(const UUID &uu, const json &j)
    : Constraint(uu, j), m_line1(j.at("line1").get<UUID>()), m_line2(j.at("line2").get<UUID>()),
      m_wrkpl(j.at("wrkpl").get<UUID>()), m_flip_normal(j.at("flip_normal").get<bool>())
{
}

json ConstraintWorkplaneNormal::serialize() const
{
    json j = Constraint::serialize();
    j["line1"] = m_line1;
    j["line2"] = m_line2;
    j["wrkpl"] = m_wrkpl;
    j["flip_normal"] = m_flip_normal;
    return j;
}

std::unique_ptr<Constraint> ConstraintWorkplaneNormal::clone() const
{
    return std::make_unique<ConstraintWorkplaneNormal>(*this);
}

std::set<EntityAndPoint> ConstraintWorkplaneNormal::get_referenced_entities_and_points() const
{
    return {{m_line1, 0}, {m_line2, 0}, {m_wrkpl, 0}};
}

static bool coincident(const glm::dvec3 &p1, const glm::dvec3 &p2)
{
    return glm::length(p1 - p2) < 1e-6;
}

std::optional<ConstraintWorkplaneNormal::UVN> ConstraintWorkplaneNormal::get_uvn(const Document &doc) const
{

    auto &line1 = doc.get_entity(m_line1);
    auto &line2 = doc.get_entity(m_line2);

    // some point of line1 must be coincident with some point on line2
    auto l1p1 = line1.get_point(1, doc);
    auto l1p2 = line1.get_point(2, doc);
    auto l2p1 = line2.get_point(1, doc);
    auto l2p2 = line2.get_point(2, doc);


    if (!coincident(l1p1, l2p1))
        std::swap(l1p1, l1p2);

    if (!coincident(l1p1, l2p1))
        std::swap(l2p1, l2p2);

    if (!coincident(l1p1, l2p1))
        std::swap(l1p1, l1p2);

    if (!coincident(l1p1, l2p1))
        return {};


    auto u = glm::normalize(l1p2 - l1p1);
    auto n = glm::normalize(glm::cross(l1p2 - l1p1, l2p2 - l2p1));
    if (m_flip_normal)
        n *= -1;
    auto v = glm::normalize(glm::cross(u, n));

    return {{u, v, n}};
}

void ConstraintWorkplaneNormal::pre_solve(Document &doc) const
{
    auto uvn = get_uvn(doc);
    if (!uvn.has_value())
        return;

    auto &wrkpl = doc.get_entity<EntityWorkplane>(m_wrkpl);

    wrkpl.m_normal = quat_from_uv(uvn->u, uvn->v);
}

void ConstraintWorkplaneNormal::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}


} // namespace dune3d

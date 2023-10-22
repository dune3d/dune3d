#include "constraint_equal_length.hpp"
#include "nlohmann/json.hpp"
#include "document.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintEqualLength::ConstraintEqualLength(const UUID &uu) : Constraint(uu)
{
}

ConstraintEqualLength::ConstraintEqualLength(const UUID &uu, const json &j)
    : Constraint(uu, j), m_entity1(j.at("entity1").get<UUID>()), m_entity2(j.at("entity2").get<UUID>()),
      m_wrkpl(j.at("wrkpl").get<UUID>())
{
}


json ConstraintEqualLength::serialize() const
{
    json j = Constraint::serialize();
    j["entity1"] = m_entity1;
    j["entity2"] = m_entity2;
    j["wrkpl"] = m_wrkpl;
    return j;
}

std::unique_ptr<Constraint> ConstraintEqualLength::clone() const
{
    return std::make_unique<ConstraintEqualLength>(*this);
}

std::set<UUID> ConstraintEqualLength::get_referenced_entities() const
{
    std::set<UUID> r = {m_entity1, m_entity2};
    if (m_wrkpl)
        r.insert(m_wrkpl);
    return r;
}

void ConstraintEqualLength::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d

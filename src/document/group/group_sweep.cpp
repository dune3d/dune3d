#include "group_sweep.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"

namespace dune3d {
GroupSweep::GroupSweep(const UUID &uu) : Group(uu)
{
}


NLOHMANN_JSON_SERIALIZE_ENUM(GroupSweep::Operation, {
                                                            {GroupSweep::Operation::DIFFERENCE, "difference"},
                                                            {GroupSweep::Operation::UNION, "union"},
                                                    })


GroupSweep::GroupSweep(const UUID &uu, const json &j)
    : Group(uu, j), m_wrkpl(j.at("wrkpl").get<UUID>()), m_source_group(j.at("source_group").get<UUID>()),
      m_operation(j.at("operation").get<Operation>())
{
}

json GroupSweep::serialize() const
{
    auto j = Group::serialize();
    j["wrkpl"] = m_wrkpl;
    j["source_group"] = m_source_group;
    j["operation"] = m_operation;
    return j;
}

const SolidModel *GroupSweep::get_solid_model() const
{
    return m_solid_model.get();
}

std::set<UUID> GroupSweep::get_referenced_entities(const Document &doc) const
{
    auto r = Group::get_referenced_entities(doc);
    r.insert(m_wrkpl);
    return r;
}

std::set<UUID> GroupSweep::get_referenced_groups(const Document &doc) const
{
    auto r = Group::get_referenced_groups(doc);
    r.insert(m_source_group);
    return r;
}

std::set<UUID> GroupSweep::get_required_entities(const Document &doc) const
{
    return {m_wrkpl};
}

std::set<UUID> GroupSweep::get_required_groups(const Document &doc) const
{
    return {m_source_group};
}

} // namespace dune3d

#include "group_solid_model_operation.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "igroup_solid_model_json.hpp"
#include "document/solid_model.hpp"

namespace dune3d {
GroupSolidModelOperation::GroupSolidModelOperation(const UUID &uu) : Group(uu)
{
}

GroupSolidModelOperation::GroupSolidModelOperation(const UUID &uu, const json &j)
    : Group(uu, j), m_source_group_argument(j.at("source_group_argument").get<UUID>()),
      m_source_group_tool(j.at("source_group_tool").get<UUID>()), m_operation(j.at("operation").get<Operation>())
{
}

json GroupSolidModelOperation::serialize() const
{
    auto j = Group::serialize();
    j["source_group_argument"] = m_source_group_argument;
    j["source_group_tool"] = m_source_group_tool;
    j["operation"] = m_operation;
    return j;
}

const SolidModel *GroupSolidModelOperation::get_solid_model() const
{
    return m_solid_model.get();
}

std::set<UUID> GroupSolidModelOperation::get_referenced_entities(const Document &doc) const
{
    auto r = Group::get_referenced_entities(doc);
    return r;
}

std::set<UUID> GroupSolidModelOperation::get_referenced_groups(const Document &doc) const
{
    return get_source_groups();
}

std::set<UUID> GroupSolidModelOperation::get_required_entities(const Document &doc) const
{
    return {};
}

std::set<UUID> GroupSolidModelOperation::get_required_groups(const Document &doc) const
{
    return get_source_groups();
}

std::set<UUID> GroupSolidModelOperation::get_source_groups() const
{
    return {m_source_group_argument, m_source_group_tool};
}


void GroupSolidModelOperation::update_solid_model(const Document &doc)
{
    m_solid_model = SolidModel::create(doc, *this);
}

std::list<GroupStatusMessage> GroupSolidModelOperation::get_messages() const
{
    auto msg = Group::get_messages();
    msg.insert(msg.end(), m_solid_model_messages.begin(), m_solid_model_messages.end());
    return msg;
}

std::unique_ptr<Group> GroupSolidModelOperation::clone() const
{
    return std::make_unique<GroupSolidModelOperation>(*this);
}

} // namespace dune3d

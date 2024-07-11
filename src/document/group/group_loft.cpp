#include "group_loft.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "igroup_solid_model_json.hpp"
#include "document/solid_model.hpp"

namespace dune3d {
GroupLoft::GroupLoft(const UUID &uu) : Group(uu)
{
}

void to_json(json &j, const GroupLoft::Source &c)
{
    j = {{"wrkpl", c.wrkpl}, {"group", c.group}};
}

void from_json(const json &j, GroupLoft::Source &c)
{
    j.at("wrkpl").get_to(c.wrkpl);
    j.at("group").get_to(c.group);
}


GroupLoft::GroupLoft(const UUID &uu, const json &j)
    : Group(uu, j), m_sources(j.at("sources").get<std::vector<Source>>()), m_ruled(j.at("ruled").get<bool>()),
      m_operation(j.at("operation").get<Operation>())
{
}

json GroupLoft::serialize() const
{
    auto j = Group::serialize();
    j["sources"] = m_sources;
    j["operation"] = m_operation;
    j["ruled"] = m_ruled;
    return j;
}

const SolidModel *GroupLoft::get_solid_model() const
{
    return m_solid_model.get();
}

std::set<UUID> GroupLoft::get_referenced_entities(const Document &doc) const
{
    auto r = Group::get_referenced_entities(doc);
    for (auto &src : m_sources) {
        r.insert(src.wrkpl);
    }
    return r;
}

std::set<UUID> GroupLoft::get_referenced_groups(const Document &doc) const
{
    return get_source_groups();
}

std::set<UUID> GroupLoft::get_required_entities(const Document &doc) const
{
    std::set<UUID> r;
    for (auto &src : m_sources) {
        r.insert(src.wrkpl);
    }
    return r;
}

std::set<UUID> GroupLoft::get_required_groups(const Document &doc) const
{
    return get_source_groups();
}

std::set<UUID> GroupLoft::get_source_groups() const
{
    std::set<UUID> r;
    for (auto &src : m_sources) {
        r.insert(src.group);
    }
    return r;
}


void GroupLoft::update_solid_model(const Document &doc)
{
    m_solid_model = SolidModel::create(doc, *this);
}

std::list<GroupStatusMessage> GroupLoft::get_messages() const
{
    auto msg = Group::get_messages();
    msg.insert(msg.end(), m_loft_messages.begin(), m_loft_messages.end());
    return msg;
}

std::unique_ptr<Group> GroupLoft::clone() const
{
    return std::make_unique<GroupLoft>(*this);
}

} // namespace dune3d

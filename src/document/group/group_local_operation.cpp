#include "group_local_operation.hpp"
#include "nlohmann/json.hpp"
#include "util/util.hpp"
#include "document/document.hpp"

namespace dune3d {
GroupLocalOperation::GroupLocalOperation(const UUID &uu) : Group(uu)
{
}

GroupLocalOperation::GroupLocalOperation(const UUID &uu, const json &j)
    : Group(uu, j), m_edges(j.at("edges").get<std::set<unsigned int>>()), m_radius(j.at("radius").get<double>())
{
}

json GroupLocalOperation::serialize() const
{
    auto j = Group::serialize();

    j["edges"] = m_edges;
    j["radius"] = m_radius;

    return j;
}

std::list<GroupStatusMessage> GroupLocalOperation::get_messages() const
{
    auto msg = Group::get_messages();
    msg.insert(msg.end(), m_local_operation_messages.begin(), m_local_operation_messages.end());
    return msg;
}

const SolidModel *GroupLocalOperation::get_solid_model() const
{
    return m_solid_model.get();
}

} // namespace dune3d

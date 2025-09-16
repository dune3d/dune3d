#include "group_local_operation.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "util/util.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "util/template_util.hpp"

namespace dune3d {
GroupLocalOperation::GroupLocalOperation(const UUID &uu) : Group(uu)
{
}

GroupLocalOperation::GroupLocalOperation(const UUID &uu, const json &j)
    : Group(uu, j), m_entities(j.at("entities")), m_edges(j.at("edges").get<std::set<unsigned int>>()), m_radius(j.at("radius").get<double>())
{
}

json GroupLocalOperation::serialize() const
{
    auto j = Group::serialize();

    // this is here for compatibility
    j["edges"] = m_edges;
    j["entities"] = m_entities;
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

bool GroupLocalOperation::entity_type_is_supported(EntityType type)
{
    using ET = EntityType;
    return !any_of(type, ET::POINT_2D);
}

} // namespace dune3d

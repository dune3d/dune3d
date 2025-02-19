#include "group_pipe.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "util/util.hpp"
#include "util/template_util.hpp"
#include "document/document.hpp"
#include "document/solid_model.hpp"
#include "document/entity/entity.hpp"

namespace dune3d {
GroupPipe::GroupPipe(const UUID &uu) : GroupSweep(uu)
{
}

GroupPipe::GroupPipe(const UUID &uu, const json &j)
    : GroupSweep(uu, j), m_spine_entities(j.at("spine_entities")), m_start_point(j.at("start_point"))
{
}

json GroupPipe::serialize() const
{
    auto j = GroupSweep::serialize();
    j["spine_entities"] = m_spine_entities;
    j["start_point"] = m_start_point;
    return j;
}

std::unique_ptr<Group> GroupPipe::clone() const
{
    return std::make_unique<GroupPipe>(*this);
}

void GroupPipe::generate(Document &doc)
{
}

void GroupPipe::update_solid_model(const Document &doc)
{
    m_solid_model = SolidModel::create(doc, *this);
}

std::set<UUID> GroupPipe::get_referenced_entities(const Document &doc) const
{
    auto r = GroupSweep::get_referenced_entities(doc);
    if (m_start_point.entity)
        r.insert(m_start_point.entity);
    r.insert(m_spine_entities.begin(), m_spine_entities.end());
    return r;
}

std::set<UUID> GroupPipe::get_required_entities(const Document &doc) const
{
    auto r = GroupSweep::get_required_entities(doc);
    if (m_start_point.entity)
        r.insert(m_start_point.entity);
    r.insert(m_spine_entities.begin(), m_spine_entities.end());
    return r;
}

bool GroupPipe::entity_type_is_supported(EntityType type)
{
    using ET = EntityType;
    return any_of(type, ET::LINE_2D, ET::ARC_2D, ET::BEZIER_2D);
}


} // namespace dune3d

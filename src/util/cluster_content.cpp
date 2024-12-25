#include "cluster_content.hpp"
#include "nlohmann/json.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/ientity_bounding_box2d.hpp"
#include "document/entity/ientity_in_workplane_set.hpp"
#include "document/constraint/constraint.hpp"
#include "util/bbox_accumulator.hpp"

namespace dune3d {
std::shared_ptr<ClusterContent> ClusterContent::create()
{
    return std::make_shared<ClusterContent>(Badge<ClusterContent>{});
}

std::shared_ptr<ClusterContent> ClusterContent::from_json(const json &j)
{
    return std::make_shared<ClusterContent>(Badge<ClusterContent>{}, j);
}

ClusterContent::ClusterContent(Badge<ClusterContent>)
{
}

ClusterContent::ClusterContent(Badge<ClusterContent>, const json &j)
{
    for (const auto &[uu, it] : j.at("entities").items()) {
        m_entities.emplace(std::piecewise_construct, std::forward_as_tuple(uu),
                           std::forward_as_tuple(Entity::new_from_json(uu, it, {})));
    }
    for (const auto &[uu, it] : j.at("constraints").items()) {
        m_constraints.emplace(std::piecewise_construct, std::forward_as_tuple(uu),
                              std::forward_as_tuple(Constraint::new_from_json(uu, it)));
    }
}

void ClusterContent::serialize(json &j) const
{
    {
        auto o = json::object();
        for (const auto &[uu, it] : m_entities) {
            if (it->m_kind == ItemKind::USER)
                o[uu] = it->serialize();
        }
        j["entities"] = o;
    }
    {
        auto o = json::object();
        for (const auto &[uu, it] : m_constraints) {
            o[uu] = it->serialize();
        }
        j["constraints"] = o;
    }
}

std::pair<glm::dvec2, glm::dvec2> ClusterContent::get_bbox() const
{
    BBoxAccumulator<glm::dvec2> acc;
    for (const auto &[uu, en] : m_entities) {
        if (auto bb = dynamic_cast<const IEntityBoundingBox2D *>(en.get()))
            acc.accumulate(bb->get_bbox());
    }
    return acc.get_or_0();
}

ClusterContent::CloneResult ClusterContent::clone_for_new_workplane(const UUID &wrkpl) const
{
    auto n = create();
    std::map<UUID, UUID> entity_xlat;
    UUID old_wrkpl;
    for (const auto &[uu, en] : m_entities) {
        if (!old_wrkpl)
            old_wrkpl = dynamic_cast<const IEntityInWorkplane &>(*en).get_workplane();
        auto en_cloned = en->clone();
        en_cloned->m_uuid = UUID::random();
        entity_xlat.emplace(uu, en_cloned->m_uuid);
        dynamic_cast<IEntityInWorkplaneSet &>(*en_cloned).set_workplane(wrkpl);
        n->m_entities.emplace(en_cloned->m_uuid, std::move(en_cloned));
    }
    for (const auto &[uu, co] : m_constraints) {
        auto co_cloned = co->clone();
        co_cloned->m_uuid = UUID::random();
        co_cloned->replace_entity(old_wrkpl, wrkpl);
        auto referenced = co_cloned->get_referenced_entities_and_points();
        for (const auto &enp : referenced) {
            if (entity_xlat.contains(enp.entity))
                co_cloned->replace_point(enp, {entity_xlat.at(enp.entity), enp.point});
        }
        n->m_constraints.emplace(co_cloned->m_uuid, std::move(co_cloned));
    }
    return {n, entity_xlat};
}


ClusterContent::~ClusterContent() = default;

} // namespace dune3d

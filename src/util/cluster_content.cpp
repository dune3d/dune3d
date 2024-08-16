#include "cluster_content.hpp"
#include "nlohmann/json.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/ientity_bounding_box2d.hpp"
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

ClusterContent::~ClusterContent() = default;

} // namespace dune3d

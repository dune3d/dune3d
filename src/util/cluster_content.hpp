#pragma once
#include <memory>
#include <glm/glm.hpp>
#include "nlohmann/json_fwd.hpp"
#include "uuid.hpp"
#include "badge.hpp"

namespace dune3d {
using json = nlohmann::json;

class Entity;
class Constraint;

class ClusterContent {
public:
    static std::shared_ptr<ClusterContent> from_json(const json &j);
    static std::shared_ptr<ClusterContent> create();
    struct CloneResult {
        std::shared_ptr<ClusterContent> content;
        std::map<UUID, UUID> entity_xlat;
    };
    CloneResult clone_for_new_workplane(const UUID &uu) const;

    std::map<UUID, std::unique_ptr<Entity>> m_entities;
    std::map<UUID, std::unique_ptr<Constraint>> m_constraints;

    ~ClusterContent();

    void serialize(json &j) const;

    std::pair<glm::dvec2, glm::dvec2> get_bbox() const;

    ClusterContent(Badge<ClusterContent>);
    ClusterContent(Badge<ClusterContent>, const json &j);
};
} // namespace dune3d

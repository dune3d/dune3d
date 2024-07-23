#pragma once
#include <memory>
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

    std::map<UUID, std::unique_ptr<Entity>> m_entities;
    std::map<UUID, std::unique_ptr<Constraint>> m_constraints;

    ~ClusterContent();

    void serialize(json &j) const;


    ClusterContent(Badge<ClusterContent>);
    ClusterContent(Badge<ClusterContent>, const json &j);
};
} // namespace dune3d

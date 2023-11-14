#pragma once
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"

namespace dune3d {

class EntityAndPoint {
public:
    UUID entity;
    unsigned int point;
    EntityAndPoint() = default;
    friend auto operator<=>(const EntityAndPoint &, const EntityAndPoint &) = default;
    friend bool operator==(const EntityAndPoint &, const EntityAndPoint &) = default;
    EntityAndPoint(const UUID& item, unsigned int p)
        : entity(item), point(p) {};
};

void to_json(nlohmann::json &j, const EntityAndPoint &ep);
void from_json(const nlohmann::json &j, EntityAndPoint &e);


} // namespace dune3d

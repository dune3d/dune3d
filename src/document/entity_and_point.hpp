#pragma once
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"

namespace dune3d {

struct EntityAndPoint {
    UUID entity;
    unsigned int point;

    friend auto operator<=>(const EntityAndPoint &, const EntityAndPoint &) = default;
    friend bool operator==(const EntityAndPoint &, const EntityAndPoint &) = default;
};

void to_json(nlohmann::json &j, const EntityAndPoint &ep);
void from_json(const nlohmann::json &j, EntityAndPoint &e);


} // namespace dune3d

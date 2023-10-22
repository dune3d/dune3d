#pragma once
#include "uuid.hpp"
#include "nlohmann/json_fwd.hpp"

namespace dune3d {
using json = nlohmann::json;
void from_json(const json &j, UUID &uu);
} // namespace dune3d

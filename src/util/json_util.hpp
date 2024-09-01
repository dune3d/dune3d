#pragma once
#include "uuid.hpp"
#include "nlohmann/json_fwd.hpp"

namespace dune3d {
using json = nlohmann::json;

class Color;

void from_json(const json &j, UUID &uu);

void to_json(json &j, const Color &c);
void from_json(const json &j, Color &c);
} // namespace dune3d

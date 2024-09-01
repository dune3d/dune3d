#include "json_util.hpp"
#include "nlohmann/json.hpp"
#include "color.hpp"

namespace dune3d {


void from_json(const json &j, UUID &uu)
{
    uu = UUID(j.get<std::string>());
}

void to_json(json &j, const Color &c)
{
    j["r"] = c.r;
    j["g"] = c.g;
    j["b"] = c.b;
}


void from_json(const json &j, Color &c)
{
    c.r = j.at("r").get<float>();
    c.g = j.at("g").get<float>();
    c.b = j.at("b").get<float>();
}

} // namespace dune3d

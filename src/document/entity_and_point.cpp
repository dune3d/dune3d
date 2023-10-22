#include "entity_and_point.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"

namespace dune3d {
using json = nlohmann::json;

void to_json(nlohmann::json &j, const EntityAndPoint &ep)
{
    j = {{"entity", ep.entity}, {"point", ep.point}};
}

void from_json(const nlohmann::json &j, EntityAndPoint &ep)
{
    j.at("entity").get_to(ep.entity);
    j.at("point").get_to(ep.point);
}


} // namespace dune3d

#include "json_util.hpp"
#include "nlohmann/json.hpp"

namespace dune3d {


void from_json(const json &j, UUID &uu)
{
    uu = UUID(j.get<std::string>());
}

} // namespace dune3d

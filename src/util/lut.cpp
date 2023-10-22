#include "lut.hpp"
#include "nlohmann/json.hpp"

namespace dune3d::detail {

std::string string_from_json(const nlohmann::json &j)
{
    return j.get<std::string>();
}

} // namespace dune3d::detail

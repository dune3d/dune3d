#include "group_array.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "util/util.hpp"

namespace dune3d {
GroupArray::GroupArray(const UUID &uu) : GroupReplicate(uu)
{
}

NLOHMANN_JSON_SERIALIZE_ENUM(GroupArray::Offset, {
                                                         {GroupArray::Offset::ONE, "one"},
                                                         {GroupArray::Offset::ZERO, "zero"},
                                                         {GroupArray::Offset::PARAM, "param"},
                                                 })

GroupArray::GroupArray(const UUID &uu, const json &j)
    : GroupReplicate(uu, j), m_count(j.at("count").get<unsigned int>()), m_offset(j.at("offset").get<Offset>())
{
}

json GroupArray::serialize() const
{
    auto j = GroupReplicate::serialize();
    j["count"] = m_count;
    j["offset"] = m_offset;
    return j;
}

} // namespace dune3d

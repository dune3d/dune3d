#include "group_mirror.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "util/util.hpp"
#include "document/entity/entity.hpp"

namespace dune3d {
GroupMirror::GroupMirror(const UUID &uu) : GroupReplicate(uu)
{
}

GroupMirror::GroupMirror(const UUID &uu, const json &j)
    : GroupReplicate(uu, j), m_include_source(j.at("include_source").get<bool>())
{
}

json GroupMirror::serialize() const
{
    auto j = GroupReplicate::serialize();
    j["include_source"] = m_include_source;
    return j;
}

void GroupMirror::post_add(Entity &new_entity, const Entity &entity, unsigned int instance) const
{
    if (instance != 1)
        return;
    new_entity.m_move_instead.clear();
    for (unsigned int pt = 0; pt <= 4; pt++) {
        if (!(entity.is_valid_point(pt) || pt == 0))
            continue;
        new_entity.m_move_instead.emplace(std::piecewise_construct, std::forward_as_tuple(pt),
                                          std::forward_as_tuple(entity.m_uuid, pt));
    }
}

} // namespace dune3d

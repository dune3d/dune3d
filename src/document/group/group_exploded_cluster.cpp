#include "group_exploded_cluster.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"

namespace dune3d {
GroupExplodedCluster::GroupExplodedCluster(const UUID &uu) : Group(uu)
{
}

GroupExplodedCluster::GroupExplodedCluster(const UUID &uu, const json &j)
    : Group(uu, j), m_cluster(j.at("cluster").get<UUID>())
{
}

json GroupExplodedCluster::serialize() const
{
    auto j = Group::serialize();
    j["cluster"] = m_cluster;
    return j;
}

std::unique_ptr<Group> GroupExplodedCluster::clone() const
{
    return std::make_unique<GroupExplodedCluster>(*this);
}

} // namespace dune3d

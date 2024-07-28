#pragma once
#include "group.hpp"

namespace dune3d {
class GroupExplodedCluster : public Group {
public:
    explicit GroupExplodedCluster(const UUID &uu);
    explicit GroupExplodedCluster(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::EXPLODED_CLUSTER;
    Type get_type() const override
    {
        return s_type;
    }

    UUID m_cluster;

    json serialize() const override;
    std::unique_ptr<Group> clone() const override;

    bool can_delete() const override
    {
        return false;
    }
};

} // namespace dune3d

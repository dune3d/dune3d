#pragma once
#include "group_replicate.hpp"

namespace dune3d {

class Document;
class SolidModel;

class GroupMirror : public GroupReplicate {
public:
    explicit GroupMirror(const UUID &uu);
    explicit GroupMirror(const UUID &uu, const json &j);

    bool m_include_source = false;

    json serialize() const override;

    bool get_mirror_arc(unsigned int instance) const override
    {
        return instance == 0;
    }

protected:
    unsigned int get_count() const override
    {
        if (m_include_source)
            return 2;
        else
            return 1;
    }

    void post_add(Entity &new_entity, const Entity &entity, unsigned int instance) const override;
};

} // namespace dune3d

#pragma once
#include "group.hpp"
#include "igroup_generate.hpp"
#include <glm/gtx/quaternion.hpp>

namespace dune3d {

class EntityWorkplane;

class GroupReference : public Group, public IGroupGenerate {
public:
    explicit GroupReference(const UUID &uu);
    explicit GroupReference(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::REFERENCE;
    Type get_type() const override
    {
        return s_type;
    }
    json serialize() const override;
    std::unique_ptr<Group> clone() const override;

    virtual void generate(Document &doc) const override;

    bool m_show_xy = true;
    bool m_show_yz = true;
    bool m_show_zx = true;

    UUID get_workplane_xy_uuid() const;
    UUID get_workplane_yz_uuid() const;
    UUID get_workplane_zx_uuid() const;

private:
    EntityWorkplane &add_workplane(Document &doc, const UUID &uu, const glm::dquat &normal) const;
};

} // namespace dune3d

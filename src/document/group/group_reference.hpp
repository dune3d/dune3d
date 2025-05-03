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
    json serialize(const Document &doc) const override;
    std::unique_ptr<Group> clone() const override;

    virtual void generate(Document &doc) override;

    bool m_show_xy = true;
    bool m_show_yz = true;
    bool m_show_zx = true;

    glm::dvec2 m_xy_size = {10, 10};
    glm::dvec2 m_yz_size = {10, 10};
    glm::dvec2 m_zx_size = {10, 10};

    UUID get_workplane_xy_uuid() const;
    UUID get_workplane_yz_uuid() const;
    UUID get_workplane_zx_uuid() const;

    bool can_delete() const override
    {
        return false;
    }

    bool can_create_entity() const override
    {
        return false;
    }

    bool can_create_constraint() const override
    {
        return false;
    }

    bool can_have_active_workplane() const override
    {
        return false;
    }

private:
    EntityWorkplane &add_workplane(Document &doc, const UUID &uu, const glm::dquat &normal,
                                   const glm::dvec2 &size) const;
};

} // namespace dune3d

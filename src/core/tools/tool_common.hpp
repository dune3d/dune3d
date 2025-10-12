#pragma once

#include "core/tool.hpp"

namespace dune3d {

class Entity;
class EntityWorkplane;
class Document;
class Group;

class ToolCommon : public ToolBase {
protected:
    using ToolBase::ToolBase;

    template <typename T> T &add_entity(const UUID &uu);
    template <typename T> T &add_entity()
    {
        return add_entity<T>(UUID::random());
    }

    template <typename T = Entity> T &get_entity(const UUID &uu);

    template <typename T> T &add_constraint(const UUID &uu);
    template <typename T> T &add_constraint()
    {
        return add_constraint<T>(UUID::random());
    }

    template <typename T> T &just_add_constraint(const UUID &uu);
    template <typename T> T &just_add_constraint()
    {
        return just_add_constraint<T>(UUID::random());
    }

    void set_current_group_solve_pending();
    void set_current_group_generate_pending();
    void set_current_group_update_solid_model_pending();
    bool current_group_has_redundant_constraints();

    void set_first_update_group_current();

    virtual bool is_force_unset_workplane();
    UUID get_workplane_uuid();
    EntityWorkplane *get_workplane();
    glm::dvec3 get_cursor_pos_for_workplane(const EntityWorkplane &wrkpl) const;

    bool can_create_entity();
    bool can_create_constraint();

    Document &get_doc();
    Group &get_group();
};
} // namespace dune3d

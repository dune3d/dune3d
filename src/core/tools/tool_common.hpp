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

    void set_current_group_solve_pending();
    void set_current_group_generate_pending();
    void set_current_group_update_solid_model_pending();

    UUID get_workplane_uuid();
    EntityWorkplane *get_workplane();

    Document &get_doc();
    Group &get_group();

    void reset_selection_after_constrain();
};
} // namespace dune3d

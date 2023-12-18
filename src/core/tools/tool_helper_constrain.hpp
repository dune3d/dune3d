#pragma once
#include "tool_common.hpp"
#include "document/constraint/constraint.hpp"
#include <optional>

namespace dune3d {

class EntityAndPoint;

class ToolHelperConstrain : public virtual ToolCommon {
protected:
    Constraint *constrain_point(const UUID &wrkpl, const EntityAndPoint &enp_to_constrain);
    Constraint *constrain_point(Constraint::Type type, const UUID &wrkpl, const EntityAndPoint &enp,
                                const EntityAndPoint &enp_to_constrain);
    std::optional<Constraint::Type> get_constraint_type();

    void set_constrain_tip(const std::string &what);
    std::string get_constrain_tip(const std::string &what);
};
} // namespace dune3d

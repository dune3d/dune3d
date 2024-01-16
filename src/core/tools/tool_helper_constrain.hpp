#pragma once
#include "tool_common.hpp"
#include <optional>

namespace dune3d {

class EntityAndPoint;
class Constraint;
enum class ConstraintType;

class ToolHelperConstrain : public virtual ToolCommon {
protected:
    Constraint *constrain_point(const UUID &wrkpl, const EntityAndPoint &enp_to_constrain);
    Constraint *constrain_point(ConstraintType type, const UUID &wrkpl, const EntityAndPoint &enp,
                                const EntityAndPoint &enp_to_constrain);
    std::optional<ConstraintType> get_constraint_type();

    void set_constrain_tip(const std::string &what);
    void update_constraint_icons(std::vector<ConstraintType> &constraint_icons);
    std::string get_constrain_tip(const std::string &what);
};
} // namespace dune3d

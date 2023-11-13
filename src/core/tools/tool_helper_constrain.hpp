#pragma once
#include "tool_common.hpp"

namespace dune3d {

class Constraint;
class EntityAndPoint;

class ToolHelperConstrain : public virtual ToolCommon {
protected:
    Constraint *constrain_point(const UUID &wrkpl, const EntityAndPoint &enp);
    void set_constrain_tip(const std::string &what);
    std::string get_constrain_tip(const std::string &what);
};
} // namespace dune3d

#pragma once
#include "tool_common_constrain.hpp"

namespace dune3d {

class Constraint;
class IConstraintDatum;
class IConstraintMovable;

class ToolCommonConstrainDatum : public ToolCommonConstrain {
public:
    using ToolCommonConstrain::ToolCommonConstrain;

    ToolResponse update(const ToolArgs &args) override;

protected:
    ToolResponse prepare_interactive(Constraint &constraint);

private:
    class IConstraintDatum *m_constraint_datum = nullptr;
    class IConstraintMovable *m_constraint_movable = nullptr;
    class EntityWorkplane *m_constraint_wrkpl = nullptr;
    bool m_have_win = false;
};
} // namespace dune3d

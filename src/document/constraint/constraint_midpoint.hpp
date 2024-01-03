#pragma once
#include "constraint_point_on_line_base.hpp"

namespace dune3d {

class Entity;

class ConstraintMidpoint : public ConstraintPointOnLineBase {
public:
    using ConstraintPointOnLineBase::ConstraintPointOnLineBase;
    static constexpr Type s_type = Type::MIDPOINT;
    Type get_type() const override
    {
        return s_type;
    }
    std::unique_ptr<Constraint> clone() const override;

    void accept(ConstraintVisitor &visitor) const override;
};

} // namespace dune3d

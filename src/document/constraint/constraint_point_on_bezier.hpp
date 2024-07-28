#pragma once
#include "constraint_point_on_line.hpp"

namespace dune3d {
class ConstraintPointOnBezier : public ConstraintPointOnLine {
public:
    using ConstraintPointOnLine::ConstraintPointOnLine;
    static constexpr Type s_type = Type::POINT_ON_BEZIER;
    Type get_type() const override
    {
        return s_type;
    }
    std::unique_ptr<Constraint> clone() const override;
    void accept(ConstraintVisitor &visitor) const override;

    void modify_to_satisfy(const Document &doc);
};
} // namespace dune3d

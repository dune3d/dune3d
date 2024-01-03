#pragma once
#include "constraint_point_on_line_base.hpp"

namespace dune3d {

class Entity;

class ConstraintPointOnLine : public ConstraintPointOnLineBase {
public:
    explicit ConstraintPointOnLine(const UUID &uu);
    explicit ConstraintPointOnLine(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::POINT_ON_LINE;
    Type get_type() const override
    {
        return s_type;
    }
    json serialize() const override;
    std::unique_ptr<Constraint> clone() const override;

    double m_val = 1;

    void accept(ConstraintVisitor &visitor) const override;
};

} // namespace dune3d

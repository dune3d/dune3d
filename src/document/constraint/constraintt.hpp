#pragma once
#include "constraint.hpp"

namespace dune3d {

class ConstraintVisitor;

template <typename T> class ConstraintT : public Constraint {
public:
    using Constraint::Constraint;
    using Base = ConstraintT<T>;

    Type get_type() const override
    {
        return T::s_type;
    }

    void accept(ConstraintVisitor &visitor) const override;
    std::unique_ptr<Constraint> clone() const override;
};

} // namespace dune3d

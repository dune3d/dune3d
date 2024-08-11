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

    bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point) override;
    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;
};

} // namespace dune3d

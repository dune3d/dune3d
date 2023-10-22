#pragma once
#include "constraint.hpp"
#include "constraint_point_distance.hpp"
#include <glm/glm.hpp>

namespace dune3d {


class ConstraintPointDistanceHV : public ConstraintPointDistanceBase {
public:
    using ConstraintPointDistanceBase::ConstraintPointDistanceBase;

    std::set<UUID> get_referenced_entities() const override;

    void accept(ConstraintVisitor &visitor) const override;
};

class ConstraintPointDistanceHorizontal : public ConstraintPointDistanceHV {
public:
    using ConstraintPointDistanceHV::ConstraintPointDistanceHV;

    static constexpr Type s_type = Type::POINT_DISTANCE_HORIZONTAL;
    Type get_type() const override
    {
        return s_type;
    }

    double measure_distance(const Document &doc) const override;

    std::unique_ptr<Constraint> clone() const override;
};

class ConstraintPointDistanceVertical : public ConstraintPointDistanceHV {
public:
    using ConstraintPointDistanceHV::ConstraintPointDistanceHV;

    static constexpr Type s_type = Type::POINT_DISTANCE_VERTICAL;
    Type get_type() const override
    {
        return s_type;
    }

    double measure_distance(const Document &doc) const override;

    std::unique_ptr<Constraint> clone() const override;
};
} // namespace dune3d

#pragma once
#include "constraint_point_distance.hpp"

namespace dune3d {
class ConstraintPointDistanceAligned : public ConstraintPointDistanceBase {
public:
    explicit ConstraintPointDistanceAligned(const UUID &uu);
    explicit ConstraintPointDistanceAligned(const UUID &uu, const json &j);

    json serialize() const override;

    static constexpr Type s_type = Type::POINT_DISTANCE_ALIGNED;
    Type get_type() const override
    {
        return s_type;
    }

    UUID m_align_entity;
    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;
    bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point) override;

    double measure_distance(const Document &doc) const override;
    glm::dvec3 get_align_vector(const Document &doc) const;
    std::string get_datum_name() const override
    {
        return "Aligned distance";
    }

    std::pair<double, double> get_datum_range() const override
    {
        return {-1e6, 1e6};
    }

    std::unique_ptr<Constraint> clone() const override;

    void accept(ConstraintVisitor &visitor) const override;
};
} // namespace dune3d

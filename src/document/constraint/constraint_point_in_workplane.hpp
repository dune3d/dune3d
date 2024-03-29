#pragma once
#include "constraint.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintPointInWorkplane : public Constraint {
public:
    explicit ConstraintPointInWorkplane(const UUID &uu);
    explicit ConstraintPointInWorkplane(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::POINT_IN_WORKPLANE;
    Type get_type() const override
    {
        return s_type;
    }
    json serialize() const override;
    std::unique_ptr<Constraint> clone() const override;

    EntityAndPoint m_point;
    UUID m_wrkpl;

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;

    void accept(ConstraintVisitor &visitor) const override;

    bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point) override;
};

} // namespace dune3d

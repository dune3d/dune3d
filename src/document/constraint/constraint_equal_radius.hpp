#pragma once
#include "constraint.hpp"

namespace dune3d {

class Entity;

class ConstraintEqualRadius : public Constraint {
public:
    explicit ConstraintEqualRadius(const UUID &uu);
    explicit ConstraintEqualRadius(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::EQUAL_RADIUS;
    Type get_type() const override
    {
        return s_type;
    }
    json serialize() const override;
    std::unique_ptr<Constraint> clone() const override;

    UUID m_entity1;
    UUID m_entity2;

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;

    void accept(ConstraintVisitor &visitor) const override;
};

} // namespace dune3d

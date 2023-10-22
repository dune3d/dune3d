#pragma once
#include "constraint.hpp"
#include "entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintPointsCoincident : public Constraint {
public:
    explicit ConstraintPointsCoincident(const UUID &uu);
    explicit ConstraintPointsCoincident(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::POINTS_COINCIDENT;
    Type get_type() const override
    {
        return s_type;
    }
    json serialize() const override;
    std::unique_ptr<Constraint> clone() const override;

    EntityAndPoint m_entity1;
    EntityAndPoint m_entity2;

    UUID m_wrkpl;

    std::set<UUID> get_referenced_entities() const override;

    void accept(ConstraintVisitor &visitor) const override;
};

} // namespace dune3d

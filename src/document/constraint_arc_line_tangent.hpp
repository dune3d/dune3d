#pragma once
#include "constraint.hpp"
#include "entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintArcLineTangent : public Constraint {
public:
    explicit ConstraintArcLineTangent(const UUID &uu);
    explicit ConstraintArcLineTangent(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::ARC_LINE_TANGENT;
    Type get_type() const override
    {
        return s_type;
    }
    json serialize() const override;
    std::unique_ptr<Constraint> clone() const override;

    EntityAndPoint m_arc;
    UUID m_line;

    std::set<UUID> get_referenced_entities() const override;

    void accept(ConstraintVisitor &visitor) const override;
};

} // namespace dune3d

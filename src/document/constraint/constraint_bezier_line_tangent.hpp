#pragma once
#include "constraint.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintBezierLineTangent : public Constraint {
public:
    explicit ConstraintBezierLineTangent(const UUID &uu);
    explicit ConstraintBezierLineTangent(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::BEZIER_LINE_TANGENT;
    Type get_type() const override
    {
        return s_type;
    }
    json serialize() const override;
    std::unique_ptr<Constraint> clone() const override;

    EntityAndPoint m_bezier;
    UUID m_line;

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;

    bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point) override;

    void accept(ConstraintVisitor &visitor) const override;
};

} // namespace dune3d

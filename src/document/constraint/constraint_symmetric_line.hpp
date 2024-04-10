#pragma once
#include "constraint.hpp"
#include "iconstraint_workplane.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintSymmetricLine : public Constraint, public IConstraintWorkplane {
public:
    explicit ConstraintSymmetricLine(const UUID &uu);
    explicit ConstraintSymmetricLine(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::SYMMETRIC_LINE;
    Type get_type() const override
    {
        return s_type;
    }

    std::unique_ptr<Constraint> clone() const override;
    json serialize() const override;

    EntityAndPoint m_entity1;
    EntityAndPoint m_entity2;
    UUID m_line;

    UUID m_wrkpl;

    const UUID &get_workplane(const Document &doc) const override
    {
        return m_wrkpl;
    }

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;

    void accept(ConstraintVisitor &visitor) const override;

    bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point) override;
};

} // namespace dune3d

#pragma once
#include "constraint.hpp"
#include "iconstraint_workplane.hpp"

namespace dune3d {

class Entity;

class ConstraintParallel : public Constraint, public IConstraintWorkplane {
public:
    explicit ConstraintParallel(const UUID &uu);
    explicit ConstraintParallel(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::PARALLEL;
    Type get_type() const override
    {
        return s_type;
    }
    json serialize() const override;
    std::unique_ptr<Constraint> clone() const override;

    UUID m_entity1;
    UUID m_entity2;
    UUID m_wrkpl;

    const UUID &get_workplane(const Document &doc) const override
    {
        return m_wrkpl;
    }

    double m_val = 1;

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;

    void accept(ConstraintVisitor &visitor) const override;
};

} // namespace dune3d

#pragma once
#include "constraint.hpp"
#include "iconstraint_workplane.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintPointOnLineBase : public Constraint, public IConstraintWorkplane {
public:
    explicit ConstraintPointOnLineBase(const UUID &uu);
    explicit ConstraintPointOnLineBase(const UUID &uu, const json &j);
    json serialize() const override;

    EntityAndPoint m_point;
    UUID m_line;
    UUID m_wrkpl;

    const UUID &get_workplane(const Document &doc) const override
    {
        return m_wrkpl;
    }

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;
    bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point) override;
    constexpr static auto s_referenced_entities_and_points_tuple =
            std::make_tuple(&ConstraintPointOnLineBase::m_point, &ConstraintPointOnLineBase::m_line,
                            &ConstraintPointOnLineBase::m_wrkpl);
};

} // namespace dune3d

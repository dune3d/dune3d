#pragma once
#include "constraintt.hpp"
#include "iconstraint_workplane.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintSymmetricLine : public ConstraintT<ConstraintSymmetricLine>, public IConstraintWorkplane {
public:
    explicit ConstraintSymmetricLine(const UUID &uu);
    explicit ConstraintSymmetricLine(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::SYMMETRIC_LINE;

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

    bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point) override;
};

} // namespace dune3d

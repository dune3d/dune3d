#pragma once
#include "constraintt.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintPointInPlane : public ConstraintT<ConstraintPointInPlane> {
public:
    explicit ConstraintPointInPlane(const UUID &uu);
    explicit ConstraintPointInPlane(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::POINT_IN_PLANE;
    json serialize() const override;

    EntityAndPoint m_point;
    UUID m_line1;
    UUID m_line2;

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;

    bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point) override;
};

} // namespace dune3d

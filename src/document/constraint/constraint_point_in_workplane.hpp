#pragma once
#include "constraintt.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintPointInWorkplane : public ConstraintT<ConstraintPointInWorkplane> {
public:
    explicit ConstraintPointInWorkplane(const UUID &uu);
    explicit ConstraintPointInWorkplane(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::POINT_IN_WORKPLANE;
    json serialize() const override;

    EntityAndPoint m_point;
    UUID m_wrkpl;

    constexpr static auto s_referenced_entities_and_points_tuple =
            std::make_tuple(&ConstraintPointInWorkplane::m_point, &ConstraintPointInWorkplane::m_wrkpl);
};

} // namespace dune3d

#pragma once
#include "constraintt.hpp"

namespace dune3d {

class Entity;

class ConstraintSameOrientation : public ConstraintT<ConstraintSameOrientation> {
public:
    explicit ConstraintSameOrientation(const UUID &uu);
    explicit ConstraintSameOrientation(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::SAME_ORIENTATION;
    json serialize() const override;

    UUID m_entity1;
    UUID m_entity2;

    double m_val = 1;

    constexpr static auto s_referenced_entities_and_points_tuple =
            std::make_tuple(&ConstraintSameOrientation::m_entity1, &ConstraintSameOrientation::m_entity2);
};

} // namespace dune3d

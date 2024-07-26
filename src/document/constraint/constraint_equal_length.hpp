#pragma once
#include "constraintt.hpp"
#include "iconstraint_workplane.hpp"

namespace dune3d {

class Entity;

class ConstraintEqualLength : public ConstraintT<ConstraintEqualLength>, public IConstraintWorkplane {
public:
    explicit ConstraintEqualLength(const UUID &uu);
    explicit ConstraintEqualLength(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::EQUAL_LENGTH;
    json serialize() const override;

    UUID m_entity1;
    UUID m_entity2;
    UUID m_wrkpl;

    const UUID &get_workplane(const Document &doc) const override
    {
        return m_wrkpl;
    }

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;
};

} // namespace dune3d

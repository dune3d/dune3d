#pragma once
#include "constraintt.hpp"
#include "iconstraint_workplane.hpp"

namespace dune3d {

class Entity;

class ConstraintParallel : public ConstraintT<ConstraintParallel>, public IConstraintWorkplane {
public:
    explicit ConstraintParallel(const UUID &uu);
    explicit ConstraintParallel(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::PARALLEL;
    json serialize() const override;

    UUID m_entity1;
    UUID m_entity2;
    UUID m_wrkpl;

    const UUID &get_workplane(const Document &doc) const override
    {
        return m_wrkpl;
    }

    double m_val = 1;

    constexpr static auto s_referenced_entities_and_points_tuple = std::make_tuple(
            &ConstraintParallel::m_entity1, &ConstraintParallel::m_entity2, &ConstraintParallel::m_wrkpl);
};

} // namespace dune3d

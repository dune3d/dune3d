#pragma once
#include "constraint.hpp"
#include "util/uuid_ptr.hpp"

namespace dune3d {

class Entity;

class ConstraintEqualLength : public Constraint {
public:
    explicit ConstraintEqualLength(const UUID &uu);
    explicit ConstraintEqualLength(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::EQUAL_LENGTH;
    Type get_type() const override
    {
        return s_type;
    }
    json serialize() const override;
    std::unique_ptr<Constraint> clone() const override;

    UUID m_entity1;
    UUID m_entity2;
    UUID m_wrkpl;

    std::set<UUID> get_referenced_entities() const override;

    void accept(ConstraintVisitor &visitor) const override;
};

} // namespace dune3d

#pragma once
#include "constraint.hpp"
#include "iconstraint_pre_solve.hpp"
#include <glm/glm.hpp>
#include <optional>

namespace dune3d {

class Entity;

class ConstraintWorkplaneNormal : public Constraint, public IConstraintPreSolve {
public:
    explicit ConstraintWorkplaneNormal(const UUID &uu);
    explicit ConstraintWorkplaneNormal(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::WORKPLANE_NORMAL;
    Type get_type() const override
    {
        return s_type;
    }
    json serialize() const override;
    std::unique_ptr<Constraint> clone() const override;

    UUID m_line1;
    UUID m_line2;

    UUID m_wrkpl;

    bool m_flip_normal = false;

    struct UVN {
        glm::dvec3 u;
        glm::dvec3 v;
        glm::dvec3 n;
    };
    std::optional<UVN> get_uvn(const Document &doc) const;


    void pre_solve(Document &doc) const override;

    std::set<UUID> get_referenced_entities() const override;

    void accept(ConstraintVisitor &visitor) const override;
};

} // namespace dune3d

#pragma once
#include "constraintt.hpp"
#include "iconstraint_pre_solve.hpp"
#include <glm/glm.hpp>
#include <optional>

namespace dune3d {

class Entity;

class ConstraintWorkplaneNormal : public ConstraintT<ConstraintWorkplaneNormal>, public IConstraintPreSolve {
public:
    explicit ConstraintWorkplaneNormal(const UUID &uu);
    explicit ConstraintWorkplaneNormal(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::WORKPLANE_NORMAL;
    json serialize() const override;

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

    constexpr static auto s_referenced_entities_and_points_tuple =
            std::make_tuple(&ConstraintWorkplaneNormal::m_line1, &ConstraintWorkplaneNormal::m_line2,
                            &ConstraintWorkplaneNormal::m_wrkpl);
};

} // namespace dune3d

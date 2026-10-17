#pragma once
#include "constraintt.hpp"
#include "document/entity/entity_and_point.hpp"
#include "iconstraint_workplane.hpp"
#include "iconstraint_movable.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class ConstraintComment : public ConstraintT<ConstraintComment>,
                          public IConstraintWorkplane,
                          public IConstraintMovable {
public:
    explicit ConstraintComment(const UUID &uu);
    explicit ConstraintComment(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::COMMENT;
    json serialize() const override;

    EntityAndPoint m_point;
    glm::dvec3 m_offset;
    UUID m_wrkpl;
    std::string m_comment;

    const UUID &get_workplane(const Document &doc) const override;

    constexpr static auto s_referenced_entities_and_points_tuple =
            std::make_tuple(&ConstraintComment::m_point, &ConstraintComment::m_wrkpl);

    glm::dvec3 get_origin(const Document &doc) const override;
    glm::dvec3 get_offset() const override;
    void set_offset(const glm::dvec3 &offset) override;
    bool offset_is_in_workplane() const override;
};

} // namespace dune3d

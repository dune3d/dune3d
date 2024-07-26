#pragma once
#include "entityt.hpp"
#include "ientity_in_workplane.hpp"
#include "ientity_radius.hpp"
#include "ientity_tangent.hpp"
#include "ientity_movable2d.hpp"
#include <glm/glm.hpp>

namespace dune3d {
class EntityArc2D : public EntityT<EntityArc2D>,
                    public IEntityInWorkplane,
                    public IEntityRadius,
                    public IEntityTangent,
                    public IEntityMovable2D {
public:
    explicit EntityArc2D(const UUID &uu);
    explicit EntityArc2D(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::ARC_2D;

    json serialize() const override;

    double get_param(unsigned int point, unsigned int axis) const override;
    void set_param(unsigned int point, unsigned int axis, double value) override;

    glm::dvec3 get_point(unsigned int point, const Document &doc) const override;
    bool is_valid_point(unsigned int point) const override;
    glm::dvec2 get_point_in_workplane(unsigned int point) const override;

    glm::dvec2 m_from;
    glm::dvec2 m_to;
    glm::dvec2 m_center;
    UUID m_wrkpl;
    bool m_no_radius_constraint = false;

    glm::dvec2 get_tangent_at_point(unsigned int point) const override;
    bool is_valid_tangent_point(unsigned int point) const override;

    const UUID &get_workplane() const override
    {
        return m_wrkpl;
    }

    void move(const Entity &last, const glm::dvec2 &delta, unsigned int point) override;

    std::string get_point_name(unsigned int point) const override;

    std::set<UUID> get_referenced_entities() const override;

    double get_radius() const override;
    glm::dvec2 get_center() const override
    {
        return m_center;
    }
};

} // namespace dune3d

#pragma once
#include "entityt.hpp"
#include "ientity_in_workplane_set.hpp"
#include "ientity_tangent.hpp"
#include "ientity_movable2d.hpp"
#include "ientity_bounding_box2d.hpp"
#include <glm/glm.hpp>

namespace dune3d {
class EntityBezier2D : public EntityT<EntityBezier2D>,
                       public IEntityInWorkplaneSet,
                       public IEntityTangent,
                       public IEntityMovable2D,
                       public IEntityBoundingBox2D {
public:
    explicit EntityBezier2D(const UUID &uu);
    explicit EntityBezier2D(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::BEZIER_2D;
    json serialize() const override;

    double get_param(unsigned int point, unsigned int axis) const override;
    void set_param(unsigned int point, unsigned int axis, double value) override;

    glm::dvec3 get_point(unsigned int point, const Document &doc) const override;
    bool is_valid_point(unsigned int point) const override;
    glm::dvec2 get_point_in_workplane(unsigned int point) const override;

    glm::dvec2 get_tangent_at_point(unsigned int point) const override;
    bool is_valid_tangent_point(unsigned int point) const override;

    glm::dvec2 m_p1;
    glm::dvec2 m_p2;
    glm::dvec2 m_c1;
    glm::dvec2 m_c2;
    UUID m_wrkpl;

    glm::dvec2 get_interpolated(double t) const;
    glm::dvec2 get_tangent(double t) const;
    glm::dvec2 get_second_derivative(double t) const;
    double get_curvature(double t) const;

    void move(const Entity &last, const glm::dvec2 &delta, unsigned int point) override;

    std::string get_point_name(unsigned int point) const override;

    const UUID &get_workplane() const override
    {
        return m_wrkpl;
    }
    void set_workplane(const UUID &uu) override
    {
        m_wrkpl = uu;
    }

    std::pair<glm::dvec2, glm::dvec2> get_bbox() const override;

    std::set<UUID> get_referenced_entities() const override;
};

} // namespace dune3d

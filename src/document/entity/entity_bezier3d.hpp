#pragma once
#include "entityt.hpp"
#include "ientity_tangent_projected.hpp"
#include "ientity_movable3d.hpp"
#include <glm/glm.hpp>

namespace dune3d {
class EntityBezier3D : public EntityT<EntityBezier3D>, public IEntityTangentProjected, public IEntityMovable3D {
public:
    explicit EntityBezier3D(const UUID &uu);
    explicit EntityBezier3D(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::BEZIER_3D;
    json serialize() const override;

    double get_param(unsigned int point, unsigned int axis) const override;
    void set_param(unsigned int point, unsigned int axis, double value) override;

    glm::dvec3 get_point(unsigned int point, const Document &doc) const override;
    bool is_valid_point(unsigned int point) const override;

    glm::dvec2 get_tangent_in_workplane(double t, const EntityWorkplane &wrkpl) const override;

    glm::dvec3 m_p1;
    glm::dvec3 m_p2;
    glm::dvec3 m_c1;
    glm::dvec3 m_c2;

    glm::dvec3 get_interpolated(double t) const;
    glm::dvec3 get_tangent(double t) const;

    void move(const Entity &last, const glm::dvec3 &delta, unsigned int point) override;

    std::string get_point_name(unsigned int point) const override;
};

} // namespace dune3d

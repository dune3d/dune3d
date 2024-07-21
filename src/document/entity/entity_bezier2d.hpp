#pragma once
#include "entity.hpp"
#include "ientity_in_workplane_set.hpp"
#include "ientity_tangent.hpp"
#include <glm/glm.hpp>

namespace dune3d {
class EntityBezier2D : public Entity, public IEntityInWorkplaneSet, public IEntityTangent {
public:
    explicit EntityBezier2D(const UUID &uu);
    explicit EntityBezier2D(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::BEZIER_2D;
    Type get_type() const override
    {
        return s_type;
    }
    json serialize() const override;
    std::unique_ptr<Entity> clone() const override;

    double get_param(unsigned int point, unsigned int axis) const override;
    void set_param(unsigned int point, unsigned int axis, double value) override;

    glm::dvec3 get_point(unsigned int point, const Document &doc) const override;
    bool is_valid_point(unsigned int point) const override;
    glm::dvec2 get_point_in_workplane(unsigned int point) const override;

    glm::dvec2 get_tangent_at_point(unsigned int point) const override;
    bool is_valid_tangent_point(unsigned int point) const override;

    void accept(EntityVisitor &visitor) const override;

    glm::dvec2 m_p1;
    glm::dvec2 m_p2;
    glm::dvec2 m_c1;
    glm::dvec2 m_c2;
    UUID m_wrkpl;

    glm::dvec2 get_interpolated(double t) const;

    std::string get_point_name(unsigned int point) const override;

    const UUID &get_workplane() const override
    {
        return m_wrkpl;
    }
    void set_workplane(const UUID &uu) override
    {
        m_wrkpl = uu;
    }

    std::set<UUID> get_referenced_entities() const override;
};

} // namespace dune3d

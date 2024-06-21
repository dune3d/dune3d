#pragma once
#include "entity.hpp"
#include "ientity_in_workplane.hpp"
#include "ientity_radius.hpp"
#include <glm/glm.hpp>

namespace dune3d {
class EntityCircle2D : public Entity, public IEntityInWorkplane, public IEntityRadius {
public:
    explicit EntityCircle2D(const UUID &uu);
    explicit EntityCircle2D(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::CIRCLE_2D;
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

    void accept(EntityVisitor &visitor) const override;

    glm::dvec2 m_center;
    double m_radius;
    UUID m_wrkpl;

    const UUID &get_workplane() const override
    {
        return m_wrkpl;
    }

    double get_radius() const override
    {
        return m_radius;
    }

    glm::dvec2 get_center() const override
    {
        return m_center;
    }

    std::string get_point_name(unsigned int point) const override;

    std::set<UUID> get_referenced_entities() const override;
};

} // namespace dune3d

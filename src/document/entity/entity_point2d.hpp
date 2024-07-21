#pragma once
#include "entityt.hpp"
#include "ientity_in_workplane_set.hpp"
#include "ientity_movable2d.hpp"
#include <glm/glm.hpp>

namespace dune3d {
class EntityPoint2D : public EntityT<EntityPoint2D>, public IEntityInWorkplaneSet, public IEntityMovable2D {
public:
    explicit EntityPoint2D(const UUID &uu);
    explicit EntityPoint2D(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::POINT_2D;

    json serialize() const override;

    double get_param(unsigned int point, unsigned int axis) const override;
    void set_param(unsigned int point, unsigned int axis, double value) override;

    glm::dvec3 get_point(unsigned int point, const Document &doc) const override;
    bool is_valid_point(unsigned int point) const override;
    glm::dvec2 get_point_in_workplane(unsigned int point) const override;

    glm::dvec2 m_p;
    UUID m_wrkpl;

    void move(const Entity &last, const glm::dvec2 &delta, unsigned int point) override;

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

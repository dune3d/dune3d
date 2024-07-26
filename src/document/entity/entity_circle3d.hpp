#pragma once
#include "entityt.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "ientity_movable3d.hpp"

namespace dune3d {
class EntityCircle3D : public EntityT<EntityCircle3D>, public IEntityMovable3D {
public:
    explicit EntityCircle3D(const UUID &uu);
    explicit EntityCircle3D(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::CIRCLE_3D;
    json serialize() const override;

    double get_param(unsigned int point, unsigned int axis) const override;
    void set_param(unsigned int point, unsigned int axis, double value) override;

    glm::dvec3 get_point(unsigned int point, const Document &doc) const override;
    bool is_valid_point(unsigned int point) const override;

    std::string get_point_name(unsigned int point) const override;

    void move(const Entity &last, const glm::dvec3 &delta, unsigned int point) override;

    glm::dvec3 m_center;
    double m_radius;
    glm::dquat m_normal;
};

} // namespace dune3d

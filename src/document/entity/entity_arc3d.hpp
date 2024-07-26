#pragma once
#include "entityt.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>


namespace dune3d {
class EntityArc3D : public EntityT<EntityArc3D> {
public:
    explicit EntityArc3D(const UUID &uu);
    static constexpr Type s_type = Type::ARC_3D;
    json serialize() const override;

    double get_param(unsigned int point, unsigned int axis) const override;
    void set_param(unsigned int point, unsigned int axis, double value) override;

    glm::dvec3 get_point(unsigned int point, const Document &doc) const override;
    bool is_valid_point(unsigned int point) const override;

    std::string get_point_name(unsigned int point) const override;

    glm::dvec3 m_from;
    glm::dvec3 m_to;
    glm::dvec3 m_center;
    glm::dquat m_normal;
};

} // namespace dune3d

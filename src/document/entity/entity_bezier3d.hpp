#pragma once
#include "entity.hpp"
#include <glm/glm.hpp>

namespace dune3d {
class EntityBezier3D : public Entity {
public:
    explicit EntityBezier3D(const UUID &uu);
    explicit EntityBezier3D(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::BEZIER_3D;
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

    void accept(EntityVisitor &visitor) const override;

    glm::dvec3 m_p1;
    glm::dvec3 m_p2;
    glm::dvec3 m_c1;
    glm::dvec3 m_c2;

    glm::dvec3 get_interpolated(double t) const;

    std::string get_point_name(unsigned int point) const override;
};

} // namespace dune3d

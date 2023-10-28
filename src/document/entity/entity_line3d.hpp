#pragma once
#include "entity.hpp"
#include <glm/glm.hpp>

namespace dune3d {
class EntityLine3D : public Entity {
public:
    explicit EntityLine3D(const UUID &uu);
    explicit EntityLine3D(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::LINE_3D;
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

    bool m_no_points = false;
};

} // namespace dune3d

#pragma once
#include "entityt.hpp"
#include <glm/glm.hpp>

namespace dune3d {
class EntityLine3D : public EntityT<EntityLine3D> {
public:
    explicit EntityLine3D(const UUID &uu);
    explicit EntityLine3D(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::LINE_3D;
    json serialize() const override;

    double get_param(unsigned int point, unsigned int axis) const override;
    void set_param(unsigned int point, unsigned int axis, double value) override;

    glm::dvec3 get_point(unsigned int point, const Document &doc) const override;
    bool is_valid_point(unsigned int point) const override;


    glm::dvec3 m_p1;
    glm::dvec3 m_p2;

    std::string get_point_name(unsigned int point) const override;

    bool m_no_points = false;
};

} // namespace dune3d

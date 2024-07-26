#pragma once
#include "entityt.hpp"
#include <glm/glm.hpp>
#include "ientity_normal.hpp"

namespace dune3d {
class EntityWorkplane : public EntityT<EntityWorkplane>, public IEntityNormal {
public:
    explicit EntityWorkplane(const UUID &uu);
    explicit EntityWorkplane(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::WORKPLANE;
    json serialize() const override;

    virtual bool has_name() const override
    {
        return true;
    }

    double get_param(unsigned int point, unsigned int axis) const override;
    void set_param(unsigned int point, unsigned int axis, double value) override;

    glm::dvec3 get_point(unsigned int point, const Document &doc) const override;
    bool is_valid_point(unsigned int point) const override;

    glm::dvec3 transform(glm::dvec2 p) const;
    glm::dvec3 transform_relative(glm::dvec2 p) const;
    glm::dvec2 project(glm::dvec3 p) const;
    glm::dvec3 project3(glm::dvec3 p) const;
    glm::dvec3 get_normal_vector() const;

    glm::dvec3 m_origin;
    glm::dquat m_normal;
    glm::dvec2 m_size;

    std::string get_point_name(unsigned int point) const override;

    void set_normal(const glm::dquat &q) override
    {
        m_normal = q;
    }
    glm::dquat get_normal() const override
    {
        return m_normal;
    }
};

} // namespace dune3d

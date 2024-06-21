#pragma once
#include "entity.hpp"
#include <glm/glm.hpp>
#include "ientity_normal.hpp"

namespace dune3d {
class EntityWorkplane : public Entity, public IEntityNormal {
public:
    explicit EntityWorkplane(const UUID &uu);
    explicit EntityWorkplane(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::WORKPLANE;
    Type get_type() const override
    {
        return s_type;
    }
    json serialize() const override;
    std::unique_ptr<Entity> clone() const override;

    virtual bool has_name() const override
    {
        return true;
    }

    double get_param(unsigned int point, unsigned int axis) const override;
    void set_param(unsigned int point, unsigned int axis, double value) override;

    glm::dvec3 get_point(unsigned int point, const Document &doc) const override;
    bool is_valid_point(unsigned int point) const override;

    void accept(EntityVisitor &visitor) const override;

    glm::dvec3 transform(glm::dvec2 p) const;
    glm::dvec3 transform_relative(glm::dvec2 p) const;
    glm::dvec2 project(glm::dvec3 p) const;
    glm::dvec3 project3(glm::dvec3 p) const;
    glm::dvec3 get_normal_vector() const;

    glm::dvec3 m_origin;
    glm::dquat m_normal;
    glm::dvec2 m_size;

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

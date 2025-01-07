#pragma once
#include "group_mirror.hpp"
#include "igroup_pre_solve.hpp"

namespace dune3d {

class GroupMirrorHV : public GroupMirror, public IGroupPreSolve {
public:
    using GroupMirror::GroupMirror;

    void pre_solve(Document &doc) const override;

    virtual glm::dvec3 get_n_vector() const = 0;

protected:
    glm::dvec3 transform(const Document &doc, const glm::dvec3 &p, unsigned int instance) const override;
    glm::dvec2 transform(const glm::dvec2 &p, unsigned int instance) const override;
    glm::dquat transform_normal(const Document &doc, const glm::dquat &q, unsigned int instance) const override;

    virtual glm::dvec2 get_vec2_mul() const = 0;
};

class GroupMirrorHorizontal : public GroupMirrorHV {
public:
    using GroupMirrorHV::GroupMirrorHV;
    static constexpr Type s_type = Type::MIRROR_HORIZONTAL;
    Type get_type() const override
    {
        return s_type;
    }

    void update_solid_model(const Document &doc) override;

    std::unique_ptr<Group> clone() const override;
    glm::dvec3 get_n_vector() const override;

protected:
    glm::dvec2 get_vec2_mul() const override;
};

class GroupMirrorVertical : public GroupMirrorHV {
public:
    using GroupMirrorHV::GroupMirrorHV;
    static constexpr Type s_type = Type::MIRROR_VERTICAL;
    Type get_type() const override
    {
        return s_type;
    }

    void update_solid_model(const Document &doc) override;

    std::unique_ptr<Group> clone() const override;
    glm::dvec3 get_n_vector() const override;

protected:
    glm::dvec2 get_vec2_mul() const override;
};

} // namespace dune3d

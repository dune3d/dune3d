#pragma once
#include "entity.hpp"
#include "ientity_in_workplane.hpp"
#include <glm/glm.hpp>

namespace dune3d {
class EntityCluster : public Entity, public IEntityInWorkplane {
public:
    explicit EntityCluster(const UUID &uu);
    explicit EntityCluster(const UUID &uu, const json &j);

    static constexpr Type s_type = Type::CLUSTER;
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

    glm::dvec2 m_origin = {0, 0};
    double m_scale_x = 1;
    double m_scale_y = 1;
    double m_angle = 0;

    bool m_lock_scale_x = false;
    bool m_lock_scale_y = false;
    bool m_lock_aspect_ratio = false;
    bool m_lock_angle = false;

    std::map<unsigned int, glm::dvec2> m_anchors_transformed;
    std::map<unsigned int, EntityAndPoint> m_anchors;
    std::map<unsigned int, EntityAndPoint> m_anchors_available;

    static constexpr unsigned int s_available_anchor_offset = 1000;

    void add_anchor(unsigned int i, const EntityAndPoint &enp);
    void remove_anchor(unsigned int i);
    void add_available_anchors();
    glm::dvec2 get_anchor_point(const EntityAndPoint &enp) const;

    glm::dvec2 transform(const glm::dvec2 &p) const;

    std::map<UUID, std::unique_ptr<Entity>> m_entities;
    std::map<UUID, std::unique_ptr<Constraint>> m_constraints;

    UUID m_wrkpl;
    UUID m_exploded_group;

    static bool is_supported_entity(const Entity &en);

    std::string get_point_name(unsigned int point) const override;

    const UUID &get_workplane() const override
    {
        return m_wrkpl;
    }

    std::set<UUID> get_referenced_entities() const override;

    virtual ~EntityCluster();
};

} // namespace dune3d

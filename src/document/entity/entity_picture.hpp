#pragma once
#include "entityt.hpp"
#include "ientity_in_workplane_set.hpp"
#include "ientity_movable2d.hpp"
#include "ientity_bounding_box2d.hpp"
#include "ientity_delete_point.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class PictureData;

class EntityPicture : public EntityT<EntityPicture>,
                      public IEntityInWorkplaneSet,
                      public IEntityMovable2D,
                      public IEntityBoundingBox2D,
                      public IEntityDeletePoint {
public:
    explicit EntityPicture(const UUID &uu);
    explicit EntityPicture(const UUID &uu, const json &j);

    static constexpr Type s_type = Type::PICTURE;
    json serialize() const override;

    double get_param(unsigned int point, unsigned int axis) const override;
    void set_param(unsigned int point, unsigned int axis, double value) override;

    glm::dvec3 get_point(unsigned int point, const Document &doc) const override;
    bool is_valid_point(unsigned int point) const override;
    glm::dvec2 get_point_in_workplane(unsigned int point) const override;

    glm::dvec2 m_origin = {0, 0};
    double m_scale_x = 1;
    double m_scale_y = 1;
    double m_angle = 0;

    bool m_lock_aspect_ratio = false;
    bool m_lock_angle = false;

    std::map<unsigned int, glm::dvec2> m_anchors;
    std::map<unsigned int, glm::dvec2> m_anchors_transformed;

    unsigned int add_anchor(const glm::dvec2 &pt);
    void update_anchor(unsigned int i, const glm::dvec2 &pt);
    void remove_anchor(unsigned int i);
    void update_builtin_anchors();
    bool delete_point(unsigned int point) override;
    bool is_user_anchor(unsigned int i) const;


    glm::dvec2 transform(const glm::dvec2 &p) const;
    glm::dvec2 untransform(const glm::dvec2 &p) const;

    UUID m_wrkpl;
    std::shared_ptr<const PictureData> m_data;
    UUID m_data_uuid;
    double m_width;
    double m_height;
    void set_data(std::shared_ptr<const PictureData> data);

    void move(const Entity &last, const glm::dvec2 &delta, unsigned int point) override;

    std::string get_point_name(unsigned int point) const override;

    const UUID &get_workplane() const override
    {
        return m_wrkpl;
    }

    void set_workplane(const UUID &uu) override
    {
        m_wrkpl = uu;
    }

    std::pair<glm::dvec2, glm::dvec2> get_bbox() const override;

    std::set<UUID> get_referenced_entities() const override;

private:
    void add_anchor(unsigned int i, const glm::dvec2 &pt);
};

} // namespace dune3d

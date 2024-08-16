#pragma once
#include "entityt.hpp"
#include "ientity_in_workplane_set.hpp"
#include "ientity_movable2d.hpp"
#include "util/cluster_content.hpp"
#include <glm/glm.hpp>

namespace dune3d {
class EntityText : public EntityT<EntityText>, public IEntityInWorkplaneSet, public IEntityMovable2D {
public:
    explicit EntityText(const UUID &uu);
    explicit EntityText(const UUID &uu, const json &j);

    static constexpr Type s_type = Type::TEXT;
    json serialize() const override;

    double get_param(unsigned int point, unsigned int axis) const override;
    void set_param(unsigned int point, unsigned int axis, double value) override;

    glm::dvec3 get_point(unsigned int point, const Document &doc) const override;
    bool is_valid_point(unsigned int point) const override;
    glm::dvec2 get_point_in_workplane(unsigned int point) const override;

    glm::dvec2 m_origin = {0, 0};
    double m_scale = 1;
    double m_angle = 0;

    bool m_lock_scale = false;
    bool m_lock_angle = false;

    std::string m_text;
    std::string m_font = "sans 10";
    std::string m_font_features;

    std::map<unsigned int, glm::dvec2> m_anchors;
    std::map<unsigned int, glm::dvec2> m_anchors_transformed;

    enum class AnchorX { LEFT, RIGHT };
    enum class AnchorY { BOTTOM, DESCEND, ASCEND, TOP, BASE };

    static constexpr unsigned int get_anchor_index(AnchorX x, AnchorY y)
    {
        const unsigned int offset = (x == AnchorX::RIGHT) ? 20 : 10;
        switch (y) {
        case AnchorY::BASE:
            return offset + 0;
        case AnchorY::BOTTOM:
            return offset + 1;
        case AnchorY::DESCEND:
            return offset + 2;
        case AnchorY::ASCEND:
            return offset + 3;
        case AnchorY::TOP:
            return offset + 4;
        }
    }


    void add_anchor(unsigned int i, const glm::dvec2 &pt);
    void clear_anchors();

    glm::dvec2 transform(const glm::dvec2 &p) const;

    std::shared_ptr<const ClusterContent> m_content;

    UUID m_wrkpl;

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

    std::set<UUID> get_referenced_entities() const override;
};

} // namespace dune3d

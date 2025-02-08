#pragma once
#include "group_circular_sweep.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Document;
class SolidModel;
class EntityWorkplane;

class GroupRevolve : public GroupCircularSweep {
public:
    explicit GroupRevolve(const UUID &uu);
    explicit GroupRevolve(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::REVOLVE;
    Type get_type() const override
    {
        return s_type;
    }

    double m_angle = 45;

    enum class Side { TOP, BOTTOM };
    bool has_side(Side side) const;

    UUID get_entity_uuid(Side side, const UUID &uu) const;

    enum class Mode { SINGLE, OFFSET, OFFSET_SYMMETRIC };
    Mode m_mode = Mode::SINGLE;
    double m_offset_mul = -1;

    double get_side_mul(Side side) const;

    void update_solid_model(const Document &doc) override;

    void generate(Document &doc) override;

    json serialize() const override;
    std::unique_ptr<Group> clone() const override;

    glm::dvec3 transform(const Document &doc, const glm::dvec3 &v, Side side) const;
    glm::dvec3 transform_normal(const Document &doc, const glm::dvec3 &v, Side side) const;

private:
    void generate(Document &doc, Side side) const;
    glm::dvec3 transform(const Document &doc, const glm::dvec2 &v, const EntityWorkplane &wrkpl, Side side) const;
};

} // namespace dune3d

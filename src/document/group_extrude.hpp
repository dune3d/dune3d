#pragma once
#include "group_sweep.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Document;
class SolidModel;

class GroupExtrude : public GroupSweep {
public:
    explicit GroupExtrude(const UUID &uu);
    explicit GroupExtrude(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::EXTRUDE;
    Type get_type() const override
    {
        return s_type;
    }

    enum class Direction { NORMAL, ARBITRARY };
    Direction m_direction = Direction::NORMAL;

    glm::dvec3 m_dvec = {0, 0, 1};
    void update_solid_model(const Document &doc) override;

    enum class Side { TOP, BOTTOM };
    bool has_side(Side side) const;

    UUID get_leader_line_uuid(Side side) const;
    UUID get_entity_uuid(Side side, const UUID &uu) const;
    UUID get_extrusion_line_uuid(Side side, const UUID &uu, unsigned int pt) const;

    enum class Mode { SINGLE, OFFSET, OFFSET_SYMMETRIC };
    Mode m_mode = Mode::SINGLE;
    double m_offset_mul = -1;

    void generate(Document &doc) const override;

    json serialize() const override;
    std::unique_ptr<Group> clone() const override;

private:
    void generate(Document &doc, Side side) const;
    double get_side_mul(Side side) const;
};

} // namespace dune3d

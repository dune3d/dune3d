#pragma once
#include "group_array.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Document;
class SolidModel;

class GroupPolarArray : public GroupArray {
public:
    explicit GroupPolarArray(const UUID &uu);
    explicit GroupPolarArray(const UUID &uu, const json &j);

    static constexpr Type s_type = Type::POLAR_ARRAY;
    Type get_type() const override
    {
        return s_type;
    }

    glm::dvec2 m_center = {0, 0};

    double m_offset_angle = 0;
    double m_delta_angle = 10;

    void update_solid_model(const Document &doc) override;

    UUID get_center_point_uuid() const;

    void generate(Document &doc) const override;

    json serialize() const override;
    std::unique_ptr<Group> clone() const override;

    double get_angle(unsigned int instance) const;

private:
    glm::dvec2 transform(const glm::dvec2 &p, unsigned int instance) const override;
    glm::dvec3 transform(const Document &doc, const glm::dvec3 &p, unsigned int instance) const override;
};

} // namespace dune3d

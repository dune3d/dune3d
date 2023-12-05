#pragma once
#include "group_array.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Document;
class SolidModel;

class GroupLinearArray : public GroupArray {
public:
    explicit GroupLinearArray(const UUID &uu);
    explicit GroupLinearArray(const UUID &uu, const json &j);

    static constexpr Type s_type = Type::LINEAR_ARRAY;
    Type get_type() const override
    {
        return s_type;
    }

    glm::dvec3 m_dvec = {1, 1, 0};

    glm::dvec3 m_offset_vec = {0, 0, 0};

    glm::dvec3 get_shift3(const Document &doc, unsigned int instance) const;
    glm::dvec2 get_shift2(unsigned int instance) const;

    void update_solid_model(const Document &doc) override;

    json serialize() const override;
    std::unique_ptr<Group> clone() const override;

private:
    glm::dvec3 get_shift(unsigned int instance) const;

    glm::dvec2 transform(const glm::dvec2 &p, unsigned int instance) const override;
    glm::dvec3 transform(const Document &doc, const glm::dvec3 &p, unsigned int instance) const override;
};

} // namespace dune3d

#include "group_linear_array.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "util/util.hpp"
#include "document/document.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_circle2d.hpp"
#include "document/entity/entity_arc2d.hpp"
#include "document/entity/entity_line3d.hpp"
#include "document/entity/entity_circle3d.hpp"
#include "document/entity/entity_arc3d.hpp"
#include "document/solid_model/solid_model.hpp"

namespace dune3d {
GroupLinearArray::GroupLinearArray(const UUID &uu) : GroupArray(uu)
{
}

GroupLinearArray::GroupLinearArray(const UUID &uu, const json &j)
    : GroupArray(uu, j), m_dvec(j.at("dvec").get<glm::dvec3>()), m_offset_vec(j.at("offset_vec").get<glm::dvec3>())
{
}

json GroupLinearArray::serialize() const
{
    auto j = GroupArray::serialize();
    j["dvec"] = m_dvec;
    j["offset_vec"] = m_offset_vec;
    return j;
}

void GroupLinearArray::update_solid_model(const Document &doc)
{
    m_solid_model = SolidModel::create(doc, *this);
}

glm::dvec3 GroupLinearArray::get_shift(unsigned int instance) const
{
    glm::dvec3 offset;
    switch (m_offset) {
    case Offset::ZERO:
        offset = {0, 0, 0};
        break;
    case Offset::ONE:
        offset = m_dvec;
        break;
    case Offset::PARAM:
        offset = m_offset_vec;
        break;
    }
    return offset + m_dvec * (double)instance;
}

glm::dvec3 GroupLinearArray::get_shift3(const Document &doc, unsigned int instance) const
{
    if (m_active_wrkpl) {
        const auto sh = get_shift2(instance);
        auto &wrkpl = doc.get_entity<EntityWorkplane>(m_active_wrkpl);
        return wrkpl.transform_relative(sh);
    }
    else {
        return get_shift(instance);
    }
}

glm::dvec2 GroupLinearArray::get_shift2(unsigned int instance) const
{
    auto sh = get_shift(instance);
    return {sh.x, sh.y};
}


glm::dvec2 GroupLinearArray::transform(const glm::dvec2 &p, unsigned int instance) const
{
    return p + get_shift2(instance);
}

glm::dvec3 GroupLinearArray::transform(const Document &doc, const glm::dvec3 &p, unsigned int instance) const
{
    return p + get_shift3(doc, instance);
}

std::unique_ptr<Group> GroupLinearArray::clone() const
{
    return std::make_unique<GroupLinearArray>(*this);
}

} // namespace dune3d

#include "group_polar_array.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "util/util.hpp"
#include "document/document.hpp"
#include "document/entity/entity_point2d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/solid_model/solid_model.hpp"
#include <glm/gtx/rotate_vector.hpp>

namespace dune3d {
GroupPolarArray::GroupPolarArray(const UUID &uu) : GroupArray(uu)
{
}

GroupPolarArray::GroupPolarArray(const UUID &uu, const json &j)
    : GroupArray(uu, j), m_center(j.at("center").get<glm::dvec2>()), m_offset_angle(j.at("offset_angle").get<double>()),
      m_delta_angle(j.at("delta_angle").get<double>())
{
}

json GroupPolarArray::serialize() const
{
    auto j = GroupArray::serialize();
    j["center"] = m_center;
    j["delta_angle"] = m_delta_angle;
    j["offset_angle"] = m_offset_angle;
    return j;
}

UUID GroupPolarArray::get_center_point_uuid() const
{
    return hash_uuids("f4a51b25-ddad-402d-ad67-fbaeeac4507e", {m_uuid});
}

void GroupPolarArray::generate(Document &doc)
{
    GroupArray::generate(doc);
    if (m_active_wrkpl) {
        auto &center = doc.get_or_add_entity<EntityPoint2D>(get_center_point_uuid());
        center.m_p = m_center;
        center.m_wrkpl = m_active_wrkpl;
        center.m_group = m_uuid;
        center.m_name = "center";
        center.m_kind = ItemKind::GENRERATED;
    }
}

void GroupPolarArray::update_solid_model(const Document &doc)
{
    m_solid_model = SolidModel::create(doc, *this);
}

double GroupPolarArray::get_angle(unsigned int instance) const
{
    double offset = 0;
    switch (m_offset) {
    case Offset::ZERO:
        break;
    case Offset::ONE:
        offset = m_delta_angle;
        break;
    case Offset::PARAM:
        offset = m_offset_angle;
        break;
    }

    return offset + m_delta_angle * instance;
}

glm::dvec2 GroupPolarArray::transform(const glm::dvec2 &p, unsigned int instance) const
{
    auto pc = p - m_center;
    return m_center + glm::rotate(pc, glm::radians(get_angle(instance)));
}

glm::dvec3 GroupPolarArray::transform(const Document &doc, const glm::dvec3 &p, unsigned int instance) const
{
    if (!m_active_wrkpl)
        return p;
    auto &wrkpl = doc.get_entity<EntityWorkplane>(m_active_wrkpl);
    const auto prj = wrkpl.project(p);
    auto shift = p - wrkpl.transform(prj);
    const auto tr = transform(prj, instance);
    return wrkpl.transform(tr) + shift;
}

std::unique_ptr<Group> GroupPolarArray::clone() const
{
    return std::make_unique<GroupPolarArray>(*this);
}

} // namespace dune3d

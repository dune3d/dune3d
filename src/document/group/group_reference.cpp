#include "group_reference.hpp"
#include "nlohmann/json.hpp"
#include "util/util.hpp"
#include "util/glm_util.hpp"
#include "document/document.hpp"

namespace dune3d {
GroupReference::GroupReference(const UUID &uu) : Group(uu)
{
}

GroupReference::GroupReference(const UUID &uu, const json &j)
    : Group(uu, j), m_show_xy(j.value("show_xy", true)), m_show_yz(j.value("show_yz", true)),
      m_show_zx(j.value("show_zx", true)),
      m_xy_size(j.value("xy_size", glm::dvec2(EntityWorkplane::s_default_size, EntityWorkplane::s_default_size))),
      m_yz_size(j.value("yz_size", glm::dvec2(EntityWorkplane::s_default_size, EntityWorkplane::s_default_size))),
      m_zx_size(j.value("zx_size", glm::dvec2(EntityWorkplane::s_default_size, EntityWorkplane::s_default_size)))
{
}

json GroupReference::serialize(const Document &doc) const
{
    json j = Group::serialize();
    j["show_xy"] = m_show_xy;
    j["show_yz"] = m_show_yz;
    j["show_zx"] = m_show_zx;

    j["xy_size"] = doc.get_entity<EntityWorkplane>(get_workplane_xy_uuid()).m_size;
    j["yz_size"] = doc.get_entity<EntityWorkplane>(get_workplane_yz_uuid()).m_size;
    j["zx_size"] = doc.get_entity<EntityWorkplane>(get_workplane_zx_uuid()).m_size;
    return j;
}

std::unique_ptr<Group> GroupReference::clone() const
{
    return std::make_unique<GroupReference>(*this);
}

UUID GroupReference::get_workplane_xy_uuid() const
{
    return hash_uuids("7f7519bb-0bff-4240-a188-d8d235e7cd68", {m_uuid});
}

UUID GroupReference::get_workplane_yz_uuid() const
{
    return hash_uuids("5411b06d-e1f8-49b5-a134-332026e96533", {m_uuid});
}

UUID GroupReference::get_workplane_zx_uuid() const
{
    return hash_uuids("8fdb3da1-0536-4338-8a07-aa1f697ccc65", {m_uuid});
}

void GroupReference::generate(Document &doc)
{
    const auto ax = glm::dvec3(1, 0, 0);
    const auto ay = glm::dvec3(0, 1, 0);
    const auto az = glm::dvec3(0, 0, 1);
    {
        auto &w = add_workplane(doc, get_workplane_xy_uuid(), quat_from_uv(ax, ay), m_xy_size);
        w.m_name = "XY";
        w.m_visible = m_show_xy;
    }
    {
        auto &w = add_workplane(doc, get_workplane_yz_uuid(), quat_from_uv(ay, az), m_yz_size);
        w.m_name = "YZ";
        w.m_visible = m_show_yz;
    }
    {
        auto &w = add_workplane(doc, get_workplane_zx_uuid(), quat_from_uv(az, ax), m_zx_size);
        w.m_name = "ZX";
        w.m_visible = m_show_zx;
    }
}

EntityWorkplane &GroupReference::add_workplane(Document &doc, const UUID &uu, const glm::dquat &normal,
                                               const glm::dvec2 &size) const
{
    bool added = false;
    auto &wrkpl = doc.get_or_add_entity<EntityWorkplane>(uu, &added);
    if (added)
        wrkpl.m_size = size;
    wrkpl.m_kind = ItemKind::GENRERATED;
    wrkpl.m_origin = glm::dvec3(0, 0, 0);
    wrkpl.m_group = m_uuid;
    wrkpl.m_normal = normal;
    return wrkpl;
}

} // namespace dune3d

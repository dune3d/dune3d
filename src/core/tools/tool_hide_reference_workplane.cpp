#include "tool_hide_reference_workplane.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/group/group_reference.hpp"

#include "tool_common_impl.hpp"
#include "util/selection_util.hpp"
#include "util/template_util.hpp"

namespace dune3d {

ToolBase::CanBegin ToolHideReferenceWorkplane::can_begin()
{
    auto enp = point_from_selection(get_doc(), m_selection, Entity::Type::WORKPLANE);
    if (!enp)
        return false;
    const auto &ref = get_doc().get_reference_group();

    return any_of(enp->entity, ref.get_workplane_xy_uuid(), ref.get_workplane_yz_uuid(), ref.get_workplane_zx_uuid());
}

ToolResponse ToolHideReferenceWorkplane::begin(const ToolArgs &args)
{
    auto enp = point_from_selection(get_doc(), m_selection, Entity::Type::WORKPLANE);
    if (!enp)
        return ToolResponse::end();

    auto &ref = get_doc().get_reference_group();
    if (enp->entity == ref.get_workplane_xy_uuid())
        ref.m_show_xy = false;
    else if (enp->entity == ref.get_workplane_yz_uuid())
        ref.m_show_yz = false;
    else if (enp->entity == ref.get_workplane_zx_uuid())
        ref.m_show_zx = false;
    else
        return ToolResponse::end();

    ref.generate(get_doc());

    return ToolResponse::commit();
}

ToolResponse ToolHideReferenceWorkplane::update(const ToolArgs &args)
{

    return ToolResponse();
}

} // namespace dune3d

#include "tool_hide_reference_workplane.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/group/group_reference.hpp"

#include "tool_common_impl.hpp"
#include "util/selection_util.hpp"

namespace dune3d {

bool *ToolHideReferenceWorkplane::get_show()
{
    auto enp = point_from_selection(get_doc(), m_selection, Entity::Type::WORKPLANE);
    if (!enp)
        return nullptr;

    auto &ref = get_doc().get_reference_group();
    if (enp->entity == ref.get_workplane_xy_uuid())
        return &ref.m_show_xy;
    else if (enp->entity == ref.get_workplane_yz_uuid())
        return &ref.m_show_yz;
    else if (enp->entity == ref.get_workplane_zx_uuid())
        return &ref.m_show_zx;

    return nullptr;
}

ToolBase::CanBegin ToolHideReferenceWorkplane::can_begin()
{
    return get_show() != nullptr;
}

ToolResponse ToolHideReferenceWorkplane::begin(const ToolArgs &args)
{
    auto s = get_show();
    if (!s)
        return ToolResponse::end();

    *s = false;

    get_doc().get_reference_group().generate(get_doc());

    return ToolResponse::commit();
}

ToolResponse ToolHideReferenceWorkplane::update(const ToolArgs &args)
{

    return ToolResponse();
}

} // namespace dune3d

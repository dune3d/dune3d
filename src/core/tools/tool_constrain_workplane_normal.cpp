#include "tool_constrain_workplane_normal.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_workplane_normal.hpp"
#include "util/selection_util.hpp"
#include "editor/editor_interface.hpp"
#include "tool_common_constrain_impl.hpp"
#include "util/action_label.hpp"
#include "util/glm_util.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include <glm/gtx/quaternion.hpp>

namespace dune3d {

UUID ToolConstrainWorkplaneNormal::get_wrkpl()
{
    auto enp = point_from_selection(get_doc(), m_selection, Entity::Type::WORKPLANE);
    if (!enp)
        return {};

    auto &wrkpl = get_entity(enp->entity);
    if (wrkpl.m_group == m_core.get_current_group() && wrkpl.m_kind == ItemKind::USER)
        return wrkpl.m_uuid;

    return {};
}

ToolBase::CanBegin ToolConstrainWorkplaneNormal::can_begin()
{
    return get_wrkpl() != UUID{};
}

ToolResponse ToolConstrainWorkplaneNormal::begin(const ToolArgs &args)
{
    m_wrkpl = get_wrkpl();
    if (!m_wrkpl)
        return ToolResponse::end();

    m_intf.enable_hover_selection();

    m_constraint = &add_constraint<ConstraintWorkplaneNormal>();
    m_constraint->m_wrkpl = m_wrkpl;
    update_tip();

    return ToolResponse();
}

void ToolConstrainWorkplaneNormal::update_tip()
{
    std::vector<ActionLabelInfo> actions;

    if (m_line1)
        actions.emplace_back(InToolActionID::LMB, "select other vector");
    else
        actions.emplace_back(InToolActionID::LMB, "select u vector");

    actions.emplace_back(InToolActionID::RMB, "end tool");

    m_intf.tool_bar_set_actions(actions);
}

ToolResponse ToolConstrainWorkplaneNormal::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            auto hsel = m_intf.get_hover_selection();

            if (!hsel.has_value())
                return ToolResponse();

            if (hsel->type != SelectableRef::Type::ENTITY)
                return ToolResponse();

            if (hsel->point != 0) {
                m_intf.tool_bar_flash("please click on a line");
                return ToolResponse();
            }

            auto &entity = get_entity(hsel->item);
            if (entity.get_type() != Entity::Type::LINE_2D && entity.get_type() != Entity::Type::LINE_3D) {
                m_intf.tool_bar_flash("please click on a line");
                return ToolResponse();
            }

            if (entity.m_group == m_core.get_current_group()) {
                m_intf.tool_bar_flash("please click on a line from a previous group");
                return ToolResponse();
            }

            if (m_line1 == UUID{}) {
                m_line1 = entity.m_uuid;
                update_tip();
            }
            else {
                m_constraint->m_line1 = m_line1;
                m_constraint->m_line2 = entity.m_uuid;
                auto uvn = m_constraint->get_uvn(get_doc());
                if (!uvn.has_value()) {
                    m_intf.tool_bar_flash("line has no coincident point with first line");
                    return ToolResponse();
                }

                {
                    auto q = quat_from_uv(uvn->u, uvn->v);
                    auto nan = glm::isnan(q);
                    if (nan.x || nan.y || nan.z || nan.w) {
                        m_intf.tool_bar_flash("line must not be parallel to first line");
                        return ToolResponse();
                    }
                }

                auto d = glm::dot(glm::vec3(uvn->n), m_intf.get_cam_normal());
                if (d > 0)
                    m_constraint->m_flip_normal = true;

                return commit();
            }

            m_selection.insert(*hsel);

        } break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
    }

    return ToolResponse();
}
} // namespace dune3d

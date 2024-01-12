#include "tool_constrain_workplane_normal.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/constraint/constraint_workplane_normal.hpp"
#include "core/tool_id.hpp"
#include <optional>
#include <iostream>
#include "util/selection_util.hpp"
#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"
#include "util/action_label.hpp"


namespace dune3d {

EntityWorkplane *ToolConstrainWorkplaneNormal::get_wrkpl()
{
    if (m_selection.size() != 1)
        return {};
    auto it = m_selection.begin();
    auto &sr1 = *it++;

    if (sr1.type != SelectableRef::Type::ENTITY)
        return {};

    auto &en1 = get_entity(sr1.item);
    if (en1.get_type() == Entity::Type::WORKPLANE) {
        auto &wrkpl = dynamic_cast<EntityWorkplane &>(en1);
        if (en1.m_group == m_core.get_current_group() && en1.m_kind == ItemKind::USER)
            return &wrkpl;
    }

    return nullptr;
}

ToolBase::CanBegin ToolConstrainWorkplaneNormal::can_begin()
{
    return get_wrkpl();
}

ToolResponse ToolConstrainWorkplaneNormal::begin(const ToolArgs &args)
{
    m_wrkpl = get_wrkpl();
    if (!m_wrkpl)
        return ToolResponse::end();

    m_intf.enable_hover_selection();

    m_constraint = &add_constraint<ConstraintWorkplaneNormal>();
    m_constraint->m_wrkpl = m_wrkpl->m_uuid;
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

            if (m_line1 == nullptr) {
                m_line1 = &entity;
                update_tip();
            }
            else {
                m_constraint->m_line1 = m_line1->m_uuid;
                m_constraint->m_line2 = entity.m_uuid;
                auto uvn = m_constraint->get_uvn(get_doc());
                if (!uvn.has_value()) {
                    m_intf.tool_bar_flash("line has no coincident point with first line");
                    return ToolResponse();
                }

                auto d = glm::dot(glm::vec3(uvn->n), m_intf.get_cam_normal());
                if (d > 0)
                    m_constraint->m_flip_normal = true;

                reset_selection_after_constrain();
                return ToolResponse::commit();
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

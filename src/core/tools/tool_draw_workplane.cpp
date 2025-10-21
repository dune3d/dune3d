#include "tool_draw_workplane.hpp"
#include "document/document.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/group/group.hpp"
#include "editor/editor_interface.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "document/constraint/constraint_lock_rotation.hpp"
#include "util/action_label.hpp"
#include "util/selection_util.hpp"
#include "util/glm_util.hpp"
#include "util/template_util.hpp"
#include "tool_common_impl.hpp"


namespace dune3d {

ToolBase::CanBegin ToolDrawWorkplane::can_begin()
{
    return can_create_entity();
}


void ToolDrawWorkplane::update_tip()
{
    std::string tip = "Orientation: " + quat_to_string(m_wrkpl->m_normal);

    std::vector<ConstraintType> constraint_icons;
    if (m_constrain) {
        tip += " " + get_constrain_tip("origin");
        update_constraint_icons(constraint_icons);
    }
    if (m_lock_rotation)
        constraint_icons.push_back(ConstraintType::LOCK_ROTATION);

    std::vector<ActionLabelInfo> actions;
    actions.reserve(9);
    actions.emplace_back(InToolActionID::LMB, "place");

    actions.emplace_back(InToolActionID::RMB, "cancel");

    if (m_constrain)
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "constraint off");
    else
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "constraint on");

    if (m_lock_rotation)
        actions.emplace_back(InToolActionID::TOGGLE_LOCK_ROTATION_CONSTRAINT, "unlock rotation");
    else
        actions.emplace_back(InToolActionID::TOGGLE_LOCK_ROTATION_CONSTRAINT, "lock rotation");

    if (m_auto_normal)
        actions.emplace_back(InToolActionID::TOGGLE_AUTO_NORMAL, "no auto rotate");
    else
        actions.emplace_back(InToolActionID::TOGGLE_AUTO_NORMAL, "auto rotate");

    actions.emplace_back(InToolActionID::ROTATE_X, InToolActionID::ROTATE_Y, InToolActionID::ROTATE_Z, "rotate");

    m_intf.set_constraint_icons(m_intf.get_cursor_pos(), {NAN, NAN, NAN}, constraint_icons);

    m_intf.tool_bar_set_actions(actions);
    m_intf.tool_bar_set_tool_tip(tip);
}


static int sgn(double x)
{
    if (x > 0)
        return 1;
    else
        return -1;
}

static glm::dquat snap_quat(const glm::quat &q)
{
    const glm::dvec3 n = glm::rotate(q, glm::vec3(0, 0, 1));
    const glm::dvec3 u = glm::rotate(q, glm::vec3(1, 0, 0));
    auto axn = abs_max_axis(n);
    glm::dvec3 ns = {0, 0, 0};
    ns[axn] = sgn(n[axn]);

    glm::dvec3 us = {0, 0, 0};
    auto other_axis1 = (axn + 1) % 3;
    auto other_axis2 = (axn + 2) % 3;
    if (std::abs(u[other_axis1]) > std::abs(u[other_axis2]))
        us[other_axis1] = sgn(u[other_axis1]);
    else
        us[other_axis2] = sgn(u[other_axis2]);

    glm::dvec3 vs = glm::cross(ns, us);
    return quat_from_uv(us, vs);
}

ToolResponse ToolDrawWorkplane::begin(const ToolArgs &args)
{
    m_intf.enable_hover_selection();


    m_wrkpl = &add_entity<EntityWorkplane>();
    m_wrkpl->m_origin = m_intf.get_cursor_pos();
    m_wrkpl->m_normal = snap_quat(m_intf.get_cam_quat());
    m_wrkpl->m_selection_invisible = true;

    update_tip();
    return ToolResponse();
}

ToolResponse ToolDrawWorkplane::update(const ToolArgs &args)
{
    if (any_of(args.type, ToolEventType::MOVE, ToolEventType::VIEW_CHANGED)) {
        m_wrkpl->m_origin = m_intf.get_cursor_pos();
        if (m_auto_normal)
            m_wrkpl->m_normal = snap_quat(m_intf.get_cam_quat());

        set_first_update_group_current();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            auto &group = get_group();
            if (!group.m_active_wrkpl)
                group.m_active_wrkpl = m_wrkpl->m_uuid;

            if (m_constrain) {
                const EntityAndPoint origin{m_wrkpl->m_uuid, 1};
                constrain_point({}, origin);
            }

            if (m_lock_rotation) {
                auto &constraint = add_constraint<ConstraintLockRotation>();
                constraint.m_entity = m_wrkpl->m_uuid;
            }

            return ToolResponse::commit();

        } break;

        case InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT: {
            m_constrain = !m_constrain;
        } break;

        case InToolActionID::TOGGLE_LOCK_ROTATION_CONSTRAINT: {
            m_lock_rotation = !m_lock_rotation;
        } break;

        case InToolActionID::TOGGLE_AUTO_NORMAL: {
            m_auto_normal = !m_auto_normal;
            if (m_auto_normal)
                m_wrkpl->m_normal = snap_quat(m_intf.get_cam_quat());
        } break;

        case InToolActionID::ROTATE_X:
        case InToolActionID::ROTATE_Y:
        case InToolActionID::ROTATE_Z: {
            const auto axis = static_cast<int>(args.action) - static_cast<int>(InToolActionID::ROTATE_X);
            glm::dvec3 a = {0, 0, 0};
            a[axis] = 1;
            const auto q = glm::angleAxis(M_PI / 2, a);
            m_auto_normal = false;
            m_lock_rotation = true;
            m_wrkpl->m_normal = m_wrkpl->m_normal * q;

        } break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
    }
    update_tip();

    return ToolResponse();
}
} // namespace dune3d

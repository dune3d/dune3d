#include "tool_common_constrain_datum.hpp"
#include "document/constraint/iconstraint_movable.hpp"
#include "document/constraint/iconstraint_workplane.hpp"
#include "document/constraint/iconstraint_datum.hpp"
#include "document/constraint/constraint.hpp"
#include "document/entity/entity_workplane.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include "editor/editor_interface.hpp"
#include "dialogs/dialogs.hpp"
#include "dialogs/enter_datum_window.hpp"


namespace dune3d {

ToolResponse ToolCommonConstrainDatum::prepare_interactive(Constraint &constraint)
{
    if (m_is_preview)
        return ToolResponse::commit();

    auto &co_wrkpl = dynamic_cast<const IConstraintWorkplane &>(constraint);
    m_constraint_datum = &dynamic_cast<IConstraintDatum &>(constraint);
    m_constraint_movable = &dynamic_cast<IConstraintMovable &>(constraint);

    if (auto wrkpl_uu = co_wrkpl.get_workplane(get_doc()))
        m_constraint_wrkpl = &get_entity<EntityWorkplane>(wrkpl_uu);

    return ToolResponse();
}

ToolResponse ToolCommonConstrainDatum::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE && !m_have_win) {
        if (m_constraint_movable->offset_is_in_workplane() && m_constraint_wrkpl) {
            auto p = m_constraint_wrkpl->project(get_cursor_pos_for_workplane(*m_constraint_wrkpl));
            m_constraint_movable->set_offset(glm::dvec3(p, 0) - m_constraint_movable->get_origin(get_doc()));
        }
        else {
            glm::dvec3 p;
            if (m_constraint_wrkpl)
                p = get_cursor_pos_for_workplane(*m_constraint_wrkpl);
            else
                p = m_intf.get_cursor_pos_for_plane(m_constraint_movable->get_origin(get_doc()),
                                                    m_intf.get_cam_normal());
            m_constraint_movable->set_offset(p - m_constraint_movable->get_origin(get_doc()));
        }
        set_first_update_group_current();
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            if (m_constraint_datum->is_measurement())
                return commit();

            double def = m_constraint_datum->get_datum();
            auto win = m_intf.get_dialogs().show_enter_datum_window("Enter " + m_constraint_datum->get_datum_name(),
                                                                    m_constraint_datum->get_datum_unit(), def);

            auto rng = m_constraint_datum->get_datum_range();
            win->set_range(rng.first, rng.second);
            m_have_win = true;
        } break;

        case InToolActionID::CANCEL:
        case InToolActionID::RMB:
            return ToolResponse::revert();

        default:;
        }
    }
    else if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::UPDATE) {
                if (auto d = dynamic_cast<const ToolDataEnterDatumWindow *>(args.data.get())) {
                    m_constraint_datum->set_datum(d->value);
                    set_current_group_solve_pending();
                    m_core.solve_current();
                }
            }
            else if (data->event == ToolDataWindow::Event::OK) {
                return commit();
            }
            else if (data->event == ToolDataWindow::Event::CLOSE) {
                return ToolResponse::revert();
            }
        }
    }
    return ToolResponse();
}


} // namespace dune3d

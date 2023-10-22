#include "tool_enter_datum.hpp"
#include "document/document.hpp"
#include "document/entity.hpp"
#include "document/constraint.hpp"
#include "document/constraint_point_distance.hpp"
#include "document/constraint_point_distance_hv.hpp"
#include "document/constraint_diameter_radius.hpp"
#include "editor_interface.hpp"
#include "dialogs/dialogs.hpp"
#include "dialogs/enter_datum_window.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

ToolResponse ToolEnterDatum::begin(const ToolArgs &args)
{
    if (m_selection.size() != 1)
        return ToolResponse::end();
    auto &sr = *m_selection.begin();

    if (sr.type != SelectableRef::Type::CONSTRAINT)
        return ToolResponse::end();

    auto &constr = get_doc().get_constraint(sr.item);


    m_constraint_point_distance = dynamic_cast<ConstraintPointDistanceBase *>(&constr);
    m_constraint_diameter_radius = dynamic_cast<ConstraintDiameterRadius *>(&constr);
    if (!m_constraint_point_distance && !m_constraint_diameter_radius)
        return ToolResponse::end();

    double def = 0;
    if (m_constraint_point_distance)
        def = m_constraint_point_distance->m_distance;
    else if (m_constraint_diameter_radius)
        def = m_constraint_diameter_radius->m_distance;

    auto win = m_intf.get_dialogs().show_enter_datum_window("Enter distance", def);

    if (dynamic_cast<ConstraintPointDistanceHV *>(m_constraint_point_distance))
        win->set_range(-1e3, 1e3);
    else
        win->set_range(0, 1e3);


    return ToolResponse();
}


ToolResponse ToolEnterDatum::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::UPDATE) {
                if (auto d = dynamic_cast<const ToolDataEnterDatumWindow *>(args.data.get())) {
                    if (m_constraint_point_distance)
                        m_constraint_point_distance->m_distance = d->value;
                    else if (m_constraint_diameter_radius)
                        m_constraint_diameter_radius->m_distance = d->value;
                    m_core.solve_current();
                }
            }
            else if (data->event == ToolDataWindow::Event::OK) {
                return ToolResponse::commit();
            }
            else if (data->event == ToolDataWindow::Event::CLOSE) {
                return ToolResponse::revert();
            }
        }
    }
    return ToolResponse();
}
} // namespace dune3d

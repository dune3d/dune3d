#include "tool_helper_constrain.hpp"
#include "editor/editor_interface.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "document/constraint/constraint_point_on_line.hpp"
#include "document/constraint/constraint_point_on_circle.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {
Constraint *ToolHelperConstrain::constrain_point(const UUID &wrkpl, const EntityAndPoint &enp_to_constrain)
{
    if (auto hsel = m_intf.get_hover_selection()) {
        if (hsel->type == SelectableRef::Type::ENTITY) {
            const auto enp = hsel->get_entity_and_point();
            if (get_doc().is_valid_point(enp)) {
                auto &constraint = add_constraint<ConstraintPointsCoincident>();
                constraint.m_entity1 = enp;
                constraint.m_entity2 = enp_to_constrain;
                constraint.m_wrkpl = wrkpl;
                m_core.solve_current();
                return &constraint;
            }
            else if (enp.point == 0) {
                auto &entity = get_entity(enp.entity);
                if (entity.get_type() == Entity::Type::LINE_2D || entity.get_type() == Entity::Type::LINE_3D) {
                    auto &constraint = add_constraint<ConstraintPointOnLine>();
                    constraint.m_line = entity.m_uuid;
                    constraint.m_point = enp_to_constrain;
                    constraint.m_wrkpl = wrkpl;
                    constraint.m_modify_to_satisfy = true;
                    m_core.solve_current();
                    return &constraint;
                }
                else if (entity.get_type() == Entity::Type::ARC_2D || entity.get_type() == Entity::Type::CIRCLE_2D) {
                    auto &constraint = add_constraint<ConstraintPointOnCircle>();
                    constraint.m_circle = entity.m_uuid;
                    constraint.m_point = enp_to_constrain;
                    constraint.m_modify_to_satisfy = true;
                    m_core.solve_current();
                    return &constraint;
                }
            }
        }
    }
    return nullptr;
}

std::string ToolHelperConstrain::get_constrain_tip(const std::string &what)
{
    if (auto hsel = m_intf.get_hover_selection()) {
        if (hsel->type == SelectableRef::Type::ENTITY) {
            const auto enp = hsel->get_entity_and_point();
            if (get_doc().is_valid_point(enp)) {
                return "constrain " + what + " on point";
            }
            else if (enp.point == 0) {
                auto &entity = get_entity(enp.entity);
                if (entity.get_type() == Entity::Type::LINE_2D || entity.get_type() == Entity::Type::LINE_3D) {
                    return "constrain " + what + " on line";
                }
                else if (entity.get_type() == Entity::Type::ARC_2D || entity.get_type() == Entity::Type::CIRCLE_2D) {
                    return "constrain " + what + " on circle";
                }
            }
        }
    }
    return "";
}


void ToolHelperConstrain::set_constrain_tip(const std::string &what)
{
    m_intf.tool_bar_set_tool_tip(get_constrain_tip(what));
}

} // namespace dune3d

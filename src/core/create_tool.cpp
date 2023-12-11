#include "core.hpp"
#include "tools/tool_draw_line_3d.hpp"
#include "tools/tool_draw_arc_2d.hpp"
#include "tools/tool_draw_point_2d.hpp"
#include "tools/tool_draw_circle_2d.hpp"
#include "tools/tool_draw_point_2d.hpp"
#include "tools/tool_draw_contour.hpp"
#include "tools/tool_delete.hpp"
#include "tools/tool_move.hpp"
#include "tools/tool_constrain_coincident.hpp"
#include "tools/tool_constrain_hv.hpp"
#include "tools/tool_constrain_distance.hpp"
#include "tools/tool_enter_datum.hpp"
#include "tools/tool_draw_workplane.hpp"
#include "tools/tool_constrain_same_orientation.hpp"
#include "tools/tool_constrain_workplane_normal.hpp"
#include "tools/tool_constrain_parallel.hpp"
#include "tools/tool_constrain_midpoint.hpp"
#include "tools/tool_constrain_equal_length.hpp"
#include "tools/tool_constrain_equal_radius.hpp"
#include "tools/tool_add_anchor.hpp"
#include "tools/tool_toggle_construction.hpp"
#include "tools/tool_import_step.hpp"
#include "tools/tool_constrain_diameter_radius.hpp"
#include "tools/tool_select_edges.hpp"
#include "tools/tool_constrain_perpendicular.hpp"
#include "tools/tool_set_workplane.hpp"
#include "tools/tool_rotate.hpp"
#include "tool_id.hpp"

namespace dune3d {

std::unique_ptr<ToolBase> Core::create_tool(ToolID tool_id, ToolBase::Flags flags)
{
    switch (tool_id) {
    case ToolID::DRAW_LINE_3D:
        return std::make_unique<ToolDrawLine3D>(tool_id, *this, m_intf, flags);

    case ToolID::DRAW_ARC_2D:
        return std::make_unique<ToolDrawArc2D>(tool_id, *this, m_intf, flags);

    case ToolID::DRAW_CIRCLE_2D:
        return std::make_unique<ToolDrawCircle2D>(tool_id, *this, m_intf, flags);

    case ToolID::DRAW_POINT_2D:
        return std::make_unique<ToolDrawPoint2D>(tool_id, *this, m_intf, flags);

    case ToolID::DRAW_CONTOUR:
    case ToolID::DRAW_CONTOUR_FROM_POINT:
        return std::make_unique<ToolDrawContour>(tool_id, *this, m_intf, flags);

    case ToolID::DELETE:
        return std::make_unique<ToolDelete>(tool_id, *this, m_intf, flags);

    case ToolID::MOVE:
        return std::make_unique<ToolMove>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_COINCIDENT:
        return std::make_unique<ToolConstrainCoincident>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_HORIZONTAL:
    case ToolID::CONSTRAIN_VERTICAL:
        return std::make_unique<ToolConstrainHV>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_DISTANCE:
    case ToolID::CONSTRAIN_DISTANCE_HORIZONTAL:
    case ToolID::CONSTRAIN_DISTANCE_VERTICAL:
        return std::make_unique<ToolConstrainDistance>(tool_id, *this, m_intf, flags);

    case ToolID::ENTER_DATUM:
        return std::make_unique<ToolEnterDatum>(tool_id, *this, m_intf, flags);

    case ToolID::DRAW_WORKPLANE:
        return std::make_unique<ToolDrawWorkplane>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_SAME_ORIENTATION:
        return std::make_unique<ToolConstrainSameOrientation>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_PARALLEL:
        return std::make_unique<ToolConstrainParallel>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_WORKPLANE_NORMAL:
        return std::make_unique<ToolConstrainWorkplaneNormal>(tool_id, *this, m_intf, flags);

    case ToolID::ADD_ANCHOR:
        return std::make_unique<ToolAddAnchor>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_MIDPOINT:
        return std::make_unique<ToolConstrainMidpoint>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_EQUAL_LENGTH:
        return std::make_unique<ToolConstrainEqualLength>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_EQUAL_RADIUS:
        return std::make_unique<ToolConstrainEqualRadius>(tool_id, *this, m_intf, flags);

    case ToolID::TOGGLE_CONSTRUCTION:
    case ToolID::UNSET_CONSTRUCTION:
    case ToolID::SET_CONSTRUCTION:
        return std::make_unique<ToolToggleConstruction>(tool_id, *this, m_intf, flags);

    case ToolID::IMPORT_STEP:
        return std::make_unique<ToolImportSTEP>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_DIAMETER:
    case ToolID::CONSTRAIN_RADIUS:
        return std::make_unique<ToolConstrainDiameterRadius>(tool_id, *this, m_intf, flags);

    case ToolID::SELECT_EDGES:
        return std::make_unique<ToolSelectEdges>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_PERPENDICULAR:
    case ToolID::CONSTRAIN_ANGLE:
        return std::make_unique<ToolConstrainPerpendicular>(tool_id, *this, m_intf, flags);

    case ToolID::SET_WORKPLANE:
    case ToolID::UNSET_WORKPLANE:
        return std::make_unique<ToolSetWorkplane>(tool_id, *this, m_intf, flags);

    case ToolID::ROTATE:
        return std::make_unique<ToolRotate>(tool_id, *this, m_intf, flags);

    default:
        throw std::runtime_error("unknown tool");
    }
}

} // namespace dune3d

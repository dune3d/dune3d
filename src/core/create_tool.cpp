#include "core.hpp"
#include "tools/tool_draw_line_3d.hpp"
#include "tools/tool_draw_point_2d.hpp"
#include "tools/tool_draw_circle_2d.hpp"
#include "tools/tool_draw_point_2d.hpp"
#include "tools/tool_draw_contour.hpp"
#include "tools/tool_delete.hpp"
#include "tools/tool_move.hpp"
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
#include "tools/tool_constrain_point_in_plane.hpp"
#include "tools/tool_add_anchor.hpp"
#include "tools/tool_move_anchor.hpp"
#include "tools/tool_toggle_construction.hpp"
#include "tools/tool_import_step.hpp"
#include "tools/tool_constrain_diameter_radius.hpp"
#include "tools/tool_select_edges.hpp"
#include "tools/tool_constrain_perpendicular.hpp"
#include "tools/tool_set_workplane.hpp"
#include "tools/tool_rotate.hpp"
#include "tools/tool_draw_regular_polygon.hpp"
#include "tools/tool_draw_rectangle.hpp"
#include "tools/tool_constrain_lock_rotation.hpp"
#include "tools/tool_flip_arc.hpp"
#include "tools/tool_constrain_point_in_workplane.hpp"
#include "tools/tool_constrain_symmetric_hv.hpp"
#include "tools/tool_constrain_symmetric_line.hpp"
#include "tools/tool_link_document.hpp"
#include "tools/tool_constrain_distance_aligned.hpp"
#include "tools/tool_import_dxf.hpp"
#include "tools/tool_create_cluster.hpp"
#include "tools/tool_add_cluster_anchor.hpp"
#include "tools/tool_draw_text.hpp"
#include "tools/tool_enter_text.hpp"
#include "tools/tool_paste.hpp"
#include "tools/tool_constrain_point_on_point.hpp"
#include "tools/tool_constrain_point_on_line.hpp"
#include "tools/tool_constrain_point_on_circle.hpp"
#include "tools/tool_constrain_point_on_bezier.hpp"
#include "tools/tool_constrain_point_line_distance.hpp"
#include "tools/tool_constrain_point_plane_distance.hpp"
#include "tools/tool_constrain_arc_line_tangent.hpp"
#include "tools/tool_constrain_bezier_line_tangent.hpp"
#include "tools/tool_constrain_curve_curve_tangent.hpp"
#include "tools/tool_constrain_line_points_perpendicular.hpp"
#include "tool_id.hpp"

namespace dune3d {

std::unique_ptr<ToolBase> Core::create_tool(ToolID tool_id, ToolBase::Flags flags)
{
    switch (tool_id) {
    case ToolID::NONE:;

    case ToolID::DRAW_LINE_3D:
        return std::make_unique<ToolDrawLine3D>(tool_id, *this, m_intf, flags);

    case ToolID::DRAW_CIRCLE_2D:
        return std::make_unique<ToolDrawCircle2D>(tool_id, *this, m_intf, flags);

    case ToolID::DRAW_POINT_2D:
        return std::make_unique<ToolDrawPoint2D>(tool_id, *this, m_intf, flags);

    case ToolID::DRAW_CONTOUR:
    case ToolID::DRAW_CONTOUR_FROM_POINT:
    case ToolID::DRAW_ARC_2D:
    case ToolID::DRAW_LINE_2D:
    case ToolID::DRAW_BEZIER_2D:
        return std::make_unique<ToolDrawContour>(tool_id, *this, m_intf, flags);

    case ToolID::DELETE:
    case ToolID::CUT:
        return std::make_unique<ToolDelete>(tool_id, *this, m_intf, flags);

    case ToolID::MOVE:
        return std::make_unique<ToolMove>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_HORIZONTAL:
    case ToolID::CONSTRAIN_HORIZONTAL_AUTO:
    case ToolID::CONSTRAIN_VERTICAL:
    case ToolID::CONSTRAIN_VERTICAL_AUTO:
        return std::make_unique<ToolConstrainHV>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_DISTANCE:
    case ToolID::CONSTRAIN_DISTANCE_HORIZONTAL:
    case ToolID::CONSTRAIN_DISTANCE_VERTICAL:
    case ToolID::MEASURE_DISTANCE:
    case ToolID::MEASURE_DISTANCE_HORIZONTAL:
    case ToolID::MEASURE_DISTANCE_VERTICAL:
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

    case ToolID::MOVE_ANCHOR:
        return std::make_unique<ToolMoveAnchor>(tool_id, *this, m_intf, flags);

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
    case ToolID::MEASURE_RADIUS:
    case ToolID::MEASURE_DIAMETER:
        return std::make_unique<ToolConstrainDiameterRadius>(tool_id, *this, m_intf, flags);

    case ToolID::SELECT_EDGES:
        return std::make_unique<ToolSelectEdges>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_PERPENDICULAR:
    case ToolID::CONSTRAIN_ANGLE:
    case ToolID::MEASURE_ANGLE:
        return std::make_unique<ToolConstrainPerpendicular>(tool_id, *this, m_intf, flags);

    case ToolID::SET_WORKPLANE:
    case ToolID::UNSET_WORKPLANE:
        return std::make_unique<ToolSetWorkplane>(tool_id, *this, m_intf, flags);

    case ToolID::ROTATE:
        return std::make_unique<ToolRotate>(tool_id, *this, m_intf, flags);

    case ToolID::DRAW_REGULAR_POLYGON:
        return std::make_unique<ToolDrawRegularPolygon>(tool_id, *this, m_intf, flags);

    case ToolID::DRAW_RECTANGLE:
        return std::make_unique<ToolDrawRectangle>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_POINT_IN_PLANE:
        return std::make_unique<ToolConstrainPointInPlane>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_LOCK_ROTATION:
        return std::make_unique<ToolConstrainLockRotation>(tool_id, *this, m_intf, flags);

    case ToolID::FLIP_ARC:
        return std::make_unique<ToolFlipArc>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_POINT_IN_WORKPLANE:
        return std::make_unique<ToolConstrainPointInWorkplane>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_SYMMETRIC_HORIZONTAL:
    case ToolID::CONSTRAIN_SYMMETRIC_VERTICAL:
        return std::make_unique<ToolConstrainSymmetricHV>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_SYMMETRIC_LINE:
        return std::make_unique<ToolConstrainSymmetricLine>(tool_id, *this, m_intf, flags);

    case ToolID::LINK_DOCUMENT:
        return std::make_unique<ToolLinkDocument>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_DISTANCE_ALIGNED:
    case ToolID::MEASURE_DISTANCE_ALIGNED:
        return std::make_unique<ToolConstrainDistanceAligned>(tool_id, *this, m_intf, flags);

    case ToolID::IMPORT_DXF:
        return std::make_unique<ToolImportDXF>(tool_id, *this, m_intf, flags);

    case ToolID::CREATE_CLUSTER:
        return std::make_unique<ToolCreateCluster>(tool_id, *this, m_intf, flags);

    case ToolID::ADD_CLUSTER_ANCHOR:
        return std::make_unique<ToolAddClusterAnchor>(tool_id, *this, m_intf, flags);

    case ToolID::DRAW_TEXT:
        return std::make_unique<ToolDrawText>(tool_id, *this, m_intf, flags);

    case ToolID::ENTER_TEXT:
        return std::make_unique<ToolEnterText>(tool_id, *this, m_intf, flags);

    case ToolID::PASTE:
        return std::make_unique<ToolPaste>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_POINT_ON_POINT:
        return std::make_unique<ToolConstrainPointOnPoint>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_POINT_ON_LINE:
        return std::make_unique<ToolConstrainPointOnLine>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_POINT_ON_CIRCLE:
        return std::make_unique<ToolConstrainPointOnCircle>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_POINT_ON_BEZIER:
        return std::make_unique<ToolConstrainPointOnBezier>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_POINT_LINE_DISTANCE:
    case ToolID::MEASURE_POINT_LINE_DISTANCE:
        return std::make_unique<ToolConstrainPointLineDistance>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_POINT_PLANE_DISTANCE:
    case ToolID::MEASURE_POINT_PLANE_DISTANCE:
        return std::make_unique<ToolConstrainPointPlaneDistance>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_ARC_LINE_TANGENT:
        return std::make_unique<ToolConstrainArcLineTangent>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_BEZIER_LINE_TANGENT:
        return std::make_unique<ToolConstrainBezierLineTangent>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_CURVE_CURVE_TANGENT:
    case ToolID::CONSTRAIN_BEZIER_BEZIER_TANGENT_SYMMETRIC:
        return std::make_unique<ToolConstrainCurveCurveTangent>(tool_id, *this, m_intf, flags);

    case ToolID::CONSTRAIN_LINE_POINTS_PERPENDICULAR:
        return std::make_unique<ToolConstrainLinePointsPerpendicular>(tool_id, *this, m_intf, flags);
    }
    throw std::runtime_error("unknown tool");
}

} // namespace dune3d

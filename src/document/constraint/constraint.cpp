#include "constraint.hpp"
#include "nlohmann/json.hpp"
#include "all_constraints.hpp"
#include "document/document.hpp"
#include "util/json_util.hpp"

namespace dune3d {

// void to_json(json& j, const UUID& uu) {
//    j = json{{"name", p.name}, {"address", p.address}, {"age", p.age}};
//}

Constraint::Constraint(const UUID &uu) : m_uuid(uu)
{
}

std::string Constraint::get_type_name(Type type)
{
    switch (type) {
    case Type::EQUAL_LENGTH:
        return "Equal length";
    case Type::EQUAL_RADIUS:
        return "Equal radius";
    case Type::PARALLEL:
        return "Parallel";
    case Type::POINTS_COINCIDENT:
        return "Points coincident";
    case Type::POINT_ON_LINE:
        return "Point on line";
    case Type::POINT_ON_CIRCLE:
        return "Point on circle";
    case Type::SAME_ORIENTATION:
        return "Same orientation";
    case Type::HORIZONTAL:
        return "Horizontal";
    case Type::VERTICAL:
        return "Vertical";
    case Type::POINT_DISTANCE:
        return "Distance";
    case Type::WORKPLANE_NORMAL:
        return "Workplane normal";
    case Type::POINT_DISTANCE_HORIZONTAL:
        return "Horizontal distance";
    case Type::POINT_DISTANCE_VERTICAL:
        return "Vertical distance";
    case Type::MIDPOINT:
        return "Midpoint";
    case Type::DIAMETER:
        return "Diameter";
    case Type::RADIUS:
        return "Radius";
    case Type::ARC_LINE_TANGENT:
        return "Arc/Line tangent";
    case Type::ARC_ARC_TANGENT:
        return "Arc/Arc tangent";
    case Type::LINE_POINTS_PERPENDICULAR:
        return "Line/points perpendicular";
    case Type::LINES_PERPENDICULAR:
        return "Perpendicular";
    case Type::LINES_ANGLE:
        return "Angle";
    default:
        return "Constraint";
    }
}

std::string Constraint::get_type_name() const
{
    return get_type_name(get_type());
}

Constraint::Constraint(const UUID &uu, const json &j) : m_uuid(uu)
{
    if (j.contains("group"))
        j.at("group").get_to(m_group);
}

NLOHMANN_JSON_SERIALIZE_ENUM(Constraint::Type,
                             {
                                     {Constraint::Type::POINTS_COINCIDENT, "points_coincident"},
                                     {Constraint::Type::PARALLEL, "parallel"},
                                     {Constraint::Type::POINT_ON_LINE, "point_on_line"},
                                     {Constraint::Type::POINT_ON_CIRCLE, "point_on_circle"},
                                     {Constraint::Type::EQUAL_LENGTH, "equal_length"},
                                     {Constraint::Type::EQUAL_RADIUS, "equal_radius"},
                                     {Constraint::Type::VERTICAL, "vertical"},
                                     {Constraint::Type::HORIZONTAL, "horizontal"},
                                     {Constraint::Type::POINT_DISTANCE, "point_distance"},
                                     {Constraint::Type::SAME_ORIENTATION, "same_orientation"},
                                     {Constraint::Type::WORKPLANE_NORMAL, "workplane_normal"},
                                     {Constraint::Type::POINT_DISTANCE_HORIZONTAL, "point_distance_horizontal"},
                                     {Constraint::Type::POINT_DISTANCE_VERTICAL, "point_distance_vertical"},
                                     {Constraint::Type::MIDPOINT, "midpoint"},
                                     {Constraint::Type::RADIUS, "radius"},
                                     {Constraint::Type::DIAMETER, "diameter"},
                                     {Constraint::Type::ARC_ARC_TANGENT, "arc_arc_tangent"},
                                     {Constraint::Type::ARC_LINE_TANGENT, "arc_line_tangent"},
                                     {Constraint::Type::LINE_POINTS_PERPENDICULAR, "line_points_perpendicular"},
                                     {Constraint::Type::LINES_PERPENDICULAR, "lines_perpendicular"},
                                     {Constraint::Type::LINES_ANGLE, "lines_angle"},
                             })

json Constraint::serialize() const
{
    return json{{"type", get_type()}, {"group", m_group}};
}

std::unique_ptr<Constraint> Constraint::new_from_json(const UUID &uu, const json &j)
{
    const auto type = j.at("type").get<Type>();
    switch (type) {
    case Type::POINTS_COINCIDENT:
        return std::make_unique<ConstraintPointsCoincident>(uu, j);
    case Type::PARALLEL:
        return std::make_unique<ConstraintParallel>(uu, j);
    case Type::POINT_ON_LINE:
        return std::make_unique<ConstraintPointOnLine>(uu, j);
    case Type::POINT_ON_CIRCLE:
        return std::make_unique<ConstraintPointOnCircle>(uu, j);
    case Type::EQUAL_LENGTH:
        return std::make_unique<ConstraintEqualLength>(uu, j);
    case Type::EQUAL_RADIUS:
        return std::make_unique<ConstraintEqualRadius>(uu, j);
    case Type::SAME_ORIENTATION:
        return std::make_unique<ConstraintSameOrientation>(uu, j);
    case Type::HORIZONTAL:
        return std::make_unique<ConstraintHorizontal>(uu, j);
    case Type::VERTICAL:
        return std::make_unique<ConstraintVertical>(uu, j);
    case Type::POINT_DISTANCE:
        return std::make_unique<ConstraintPointDistance>(uu, j);
    case Type::WORKPLANE_NORMAL:
        return std::make_unique<ConstraintWorkplaneNormal>(uu, j);
    case Type::POINT_DISTANCE_HORIZONTAL:
        return std::make_unique<ConstraintPointDistanceHorizontal>(uu, j);
    case Type::POINT_DISTANCE_VERTICAL:
        return std::make_unique<ConstraintPointDistanceVertical>(uu, j);
    case Type::MIDPOINT:
        return std::make_unique<ConstraintMidpoint>(uu, j);
    case Type::DIAMETER:
        return std::make_unique<ConstraintDiameter>(uu, j);
    case Type::RADIUS:
        return std::make_unique<ConstraintRadius>(uu, j);
    case Type::ARC_LINE_TANGENT:
        return std::make_unique<ConstraintArcLineTangent>(uu, j);
    case Type::ARC_ARC_TANGENT:
        return std::make_unique<ConstraintArcArcTangent>(uu, j);
    case Type::LINE_POINTS_PERPENDICULAR:
        return std::make_unique<ConstraintLinePointsPerpendicular>(uu, j);
    case Type::LINES_PERPENDICULAR:
        return std::make_unique<ConstraintLinesPerpendicular>(uu, j);
    case Type::LINES_ANGLE:
        return std::make_unique<ConstraintLinesAngle>(uu, j);
    }
    throw std::runtime_error("unknown constraint type");
}

bool Constraint::is_valid(const Document &doc) const
{
    for (auto &entity : get_referenced_entities()) {
        if (!doc.m_entities.contains(entity))
            return false;
    }
    return true;
}

Constraint::~Constraint() = default;
} // namespace dune3d

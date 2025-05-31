#pragma once
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include "document/item_kind.hpp"
#include "constraint_visitor.hpp"
#include "document/entity/entity_and_point.hpp"
#include <memory>
#include <set>

namespace dune3d {
using json = nlohmann::json;

class Document;
class EntityAndPoint;

enum class ConstraintType {
    INVALID,
    POINTS_COINCIDENT,
    PARALLEL,
    POINT_ON_LINE,
    POINT_ON_CIRCLE,
    EQUAL_LENGTH,
    EQUAL_RADIUS,
    SAME_ORIENTATION,
    HORIZONTAL,
    VERTICAL,
    POINT_DISTANCE,
    POINT_DISTANCE_HORIZONTAL,
    POINT_DISTANCE_VERTICAL,
    POINT_DISTANCE_ALIGNED,
    WORKPLANE_NORMAL,
    MIDPOINT,
    DIAMETER,
    RADIUS,
    ARC_LINE_TANGENT,
    ARC_ARC_TANGENT,
    LINE_POINTS_PERPENDICULAR,
    LINES_PERPENDICULAR,
    LINES_ANGLE,
    POINT_IN_PLANE,
    POINT_LINE_DISTANCE,
    POINT_PLANE_DISTANCE,
    LOCK_ROTATION,
    POINT_IN_WORKPLANE,
    SYMMETRIC_HORIZONTAL,
    SYMMETRIC_VERTICAL,
    SYMMETRIC_LINE,
    BEZIER_LINE_TANGENT,
    BEZIER_BEZIER_TANGENT_SYMMETRIC,
    POINT_ON_BEZIER,
    BEZIER_BEZIER_SAME_CURVATURE,
};

class Constraint {
public:
    UUID m_uuid;
    using Type = ConstraintType;
    virtual Type get_type() const = 0;
    static std::string get_type_name(Type type);
    std::string get_type_name() const;

    template <typename... Args> bool of_type(Args &&...args) const
    {
        const auto type = get_type();
        return ((type == args) || ...);
    }

    virtual ~Constraint();
    virtual json serialize() const;

    static std::unique_ptr<Constraint> new_from_json(const UUID &uu, const json &j);
    virtual std::unique_ptr<Constraint> clone() const = 0;

    bool is_valid(const Document &doc) const;

    virtual void accept(ConstraintVisitor &visitor) const = 0;

    UUID m_group;

    std::set<UUID> get_referenced_entities() const;
    virtual std::set<EntityAndPoint> get_referenced_entities_and_points() const = 0;

    bool m_modify_to_satisfy = false;

    // returns true if replaced
    virtual bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point) = 0;
    bool replace_entity(const UUID &old_entity, const UUID &new_entity);

protected:
    explicit Constraint(const UUID &uu);
    explicit Constraint(const UUID &uu, const json &j);
};

} // namespace dune3d

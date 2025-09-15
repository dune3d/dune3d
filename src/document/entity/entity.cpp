#include "entity.hpp"
#include "nlohmann/json.hpp"
#include "all_entities.hpp"
#include "util/json_util.hpp"
#include "document/document.hpp"
#include "document/group/group.hpp"
#include "document/constraint/constraint.hpp"
#include "document/constraint/iconstraint_datum.hpp"

namespace dune3d {

Entity::Entity(const UUID &uu) : m_uuid(uu)
{
}

std::string Entity::get_type_name(Type type, TypeNameStyle style)
{
    switch (type) {
    case Type::LINE_2D:
        if (style == TypeNameStyle::WITHOUT_WORKPLANE)
            return "Line";
        return "Line in workplane";
    case Type::LINE_3D:
        return "Line in 3D";
    case Type::ARC_2D:
        if (style == TypeNameStyle::WITHOUT_WORKPLANE)
            return "Arc";
        return "Arc in workplane";
    case Type::ARC_3D:
        return "Arc in 3D";
    case Type::CIRCLE_2D:
        if (style == TypeNameStyle::WITHOUT_WORKPLANE)
            return "Circle";
        return "Circle in workplane";
    case Type::CIRCLE_3D:
        return "Circle in 3D";
    case Type::STEP:
        return "STEP model";
    case Type::WORKPLANE:
        return "Workplane";
    case Type::POINT_2D:
        if (style == TypeNameStyle::WITHOUT_WORKPLANE)
            return "Point";
        return "Point in workplane";
    case Type::DOCUMENT:
        return "Document";
    case Type::BEZIER_2D:
        if (style == TypeNameStyle::WITHOUT_WORKPLANE)
            return "Bezier curve";
        return "Bezier curve in workplane";
    case Type::BEZIER_3D:
        return "Bezier curve in 3D";
    case Type::CLUSTER:
        return "Cluster";
    case Type::TEXT:
        return "Text";
    case Type::PICTURE:
        return "Picture";
    default:
        return "Entity";
    }
}

std::string Entity::get_type_name(TypeNameStyle style) const
{
    return get_type_name(get_type(), style);
}

std::string Entity::get_type_name_plural(Type type, TypeNameStyle style)
{
    switch (type) {
    case Type::LINE_2D:
        if (style == TypeNameStyle::WITHOUT_WORKPLANE)
            return "Lines";
        return "Lines in workplane";
    case Type::LINE_3D:
        return "Lines in 3D";
    case Type::ARC_2D:
        if (style == TypeNameStyle::WITHOUT_WORKPLANE)
            return "Arcs";
        return "Arcs in workplane";
    case Type::ARC_3D:
        return "Arcs in 3D";
    case Type::CIRCLE_2D:
        if (style == TypeNameStyle::WITHOUT_WORKPLANE)
            return "Circles";
        return "Circles in workplane";
    case Type::CIRCLE_3D:
        return "Circles in 3D";
    case Type::STEP:
        return "STEP models";
    case Type::WORKPLANE:
        return "Workplanes";
    case Type::POINT_2D:
        if (style == TypeNameStyle::WITHOUT_WORKPLANE)
            return "Points";
        return "Points in workplane";
    case Type::DOCUMENT:
        return "Documents";
    case Type::BEZIER_2D:
        if (style == TypeNameStyle::WITHOUT_WORKPLANE)
            return "Bezier curves";
        return "Bezier curves in workplane";
    case Type::BEZIER_3D:
        return "Bezier curves in 3D";
    case Type::CLUSTER:
        return "Clusters";
    case Type::TEXT:
        return "Texts";
    case Type::PICTURE:
        return "Pictures";
    default:
        return "Entities";
    }
}

std::string Entity::get_type_name_plural(TypeNameStyle style) const
{
    return get_type_name_plural(get_type(), style);
}

std::string Entity::get_type_name_for_n(Type type, std::size_t n, TypeNameStyle style)
{
    return n == 1 ? get_type_name(type, style) : get_type_name_plural(type, style);
}

std::string Entity::get_type_name_for_n(std::size_t n, TypeNameStyle style) const
{
    return get_type_name_for_n(get_type(), n, style);
}

Entity::Entity(const UUID &uu, const json &j)
    : m_uuid(uu), m_name(j.value("name", "")), m_construction(j.value("construction", false))
{
    if (j.contains("group"))
        j.at("group").get_to(m_group);
}

NLOHMANN_JSON_SERIALIZE_ENUM(Entity::Type, {
                                                   {Entity::Type::INVALID, "invalid"},
                                                   {Entity::Type::LINE_3D, "line_3d"},
                                                   {Entity::Type::LINE_2D, "line_2d"},
                                                   {Entity::Type::ARC_2D, "arc_2d"},
                                                   {Entity::Type::ARC_3D, "arc_3d"},
                                                   {Entity::Type::CIRCLE_2D, "circle_2d"},
                                                   {Entity::Type::CIRCLE_3D, "circle_3d"},
                                                   {Entity::Type::WORKPLANE, "workplane"},
                                                   {Entity::Type::STEP, "step"},
                                                   {Entity::Type::POINT_2D, "point_2d"},
                                                   {Entity::Type::DOCUMENT, "document"},
                                                   {Entity::Type::BEZIER_2D, "bezier_2d"},
                                                   {Entity::Type::BEZIER_3D, "bezier_3d"},
                                                   {Entity::Type::CLUSTER, "cluster"},
                                                   {Entity::Type::TEXT, "text"},
                                                   {Entity::Type::PICTURE, "picture"},
                                           })

json Entity::serialize_type(Type type)
{
    return type;
}

json Entity::serialize() const
{
    return json{{"type", get_type()}, {"group", m_group}, {"construction", m_construction}, {"name", m_name}};
}

std::unique_ptr<Entity> Entity::new_from_json(const UUID &uu, const json &j,
                                              const std::filesystem::path &containing_dir)
{
    const auto type = j.at("type").get<Type>();
    switch (type) {
    case Type::LINE_3D:
        return std::make_unique<EntityLine3D>(uu, j);
    case Type::LINE_2D:
        return std::make_unique<EntityLine2D>(uu, j);
    case Type::ARC_2D:
        return std::make_unique<EntityArc2D>(uu, j);
    case Type::CIRCLE_2D:
        return std::make_unique<EntityCircle2D>(uu, j);
    case Type::ARC_3D:
    case Type::CIRCLE_3D:
    case Type::BEZIER_3D:
        throw std::runtime_error("not supported");
    case Type::WORKPLANE:
        return std::make_unique<EntityWorkplane>(uu, j);
    case Type::STEP:
        return std::make_unique<EntitySTEP>(uu, j, containing_dir);
    case Type::POINT_2D:
        return std::make_unique<EntityPoint2D>(uu, j);
    case Type::DOCUMENT:
        return std::make_unique<EntityDocument>(uu, j);
    case Type::BEZIER_2D:
        return std::make_unique<EntityBezier2D>(uu, j);
    case Type::CLUSTER:
        return std::make_unique<EntityCluster>(uu, j);
    case Type::TEXT:
        return std::make_unique<EntityText>(uu, j);
    case Type::PICTURE:
        return std::make_unique<EntityPicture>(uu, j);
    case Type::INVALID:
        throw std::runtime_error("unknown entity type " + j.at("type").get<std::string>());
    }
    throw std::runtime_error("unknown entity type");
}

glm::dvec3 Entity::get_point(unsigned int point, const Document &doc) const
{
    return {NAN, NAN, NAN};
}

bool Entity::is_valid_point(unsigned int point) const
{
    return false;
}

std::set<UUID> Entity::get_referenced_entities() const
{
    if (m_generated_from)
        return {m_generated_from};
    return {};
}

bool Entity::can_delete(const Document &doc) const
{
    auto &group = doc.get_group(m_group);
    return group.get_type() != Group::Type::REFERENCE;
}

bool Entity::can_move(const Document &doc) const
{
    auto &group = doc.get_group(m_group);
    return group.get_type() != Group::Type::REFERENCE;
}

std::set<const Constraint *> Entity::get_constraints(const Document &doc) const
{
    std::set<const Constraint *> constraints;
    for (const auto &[uu, constraint] : doc.m_constraints) {
        if (constraint->m_group != m_group)
            continue;
        if (auto iconstraint_datum = dynamic_cast<const IConstraintDatum *>(constraint.get())) {
            if (iconstraint_datum->is_measurement())
                continue;
        }
        if (constraint->get_referenced_entities().contains(m_uuid))
            constraints.insert(constraint.get());
    }
    return constraints;
}

std::set<ConstraintType> Entity::get_constraint_types(const Document &doc) const
{
    std::set<Constraint::Type> types;
    for (const auto constraint : get_constraints(doc)) {
        types.insert(constraint->get_type());
    }
    return types;
}


Entity::~Entity() = default;
} // namespace dune3d

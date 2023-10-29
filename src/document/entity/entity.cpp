#include "entity.hpp"
#include "nlohmann/json.hpp"
#include "all_entities.hpp"
#include "util/json_util.hpp"
#include "document/document.hpp"
#include "document/group/group.hpp"

namespace dune3d {

Entity::Entity(const UUID &uu) : m_uuid(uu)
{
}

std::string Entity::get_type_name(Type type)
{
    switch (type) {
    case Type::LINE_2D:
        return "Line in workplane";
    case Type::LINE_3D:
        return "Line in 3D";
    case Type::ARC_2D:
        return "Arc in workplane";
    case Type::ARC_3D:
        return "Arc in 3D";
    case Type::CIRCLE_2D:
        return "Circle in workplane";
    case Type::CIRCLE_3D:
        return "Circle in 3D";
    case Type::STEP:
        return "STEP model";
    case Type::WORKPLANE:
        return "Workplane";
    default:
        return "Entity";
    }
}

std::string Entity::get_type_name() const
{
    return get_type_name(get_type());
}

Entity::Entity(const UUID &uu, const json &j) : m_uuid(uu), m_construction(j.value("construction", false))
{
    if (j.contains("group"))
        j.at("group").get_to(m_group);
}

NLOHMANN_JSON_SERIALIZE_ENUM(Entity::Type, {
                                                   {Entity::Type::LINE_3D, "line_3d"},
                                                   {Entity::Type::LINE_2D, "line_2d"},
                                                   {Entity::Type::ARC_2D, "arc_2d"},
                                                   {Entity::Type::ARC_3D, "arc_3d"},
                                                   {Entity::Type::CIRCLE_2D, "circle_2d"},
                                                   {Entity::Type::CIRCLE_3D, "circle_3d"},
                                                   {Entity::Type::WORKPLANE, "workplane"},
                                                   {Entity::Type::STEP, "step"},
                                           })

json Entity::serialize() const
{
    return json{{"type", get_type()}, {"group", m_group}, {"construction", m_construction}};
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
        throw std::runtime_error("not supported");
    case Type::WORKPLANE:
        return std::make_unique<EntityWorkplane>(uu, j);
    case Type::STEP:
        return std::make_unique<EntitySTEP>(uu, j, containing_dir);
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

Entity::~Entity() = default;
} // namespace dune3d

#pragma once
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include "document/item_kind.hpp"
#include "entity_and_point.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <set>
#include <filesystem>

namespace dune3d {

class Document;
class EntityVisitor;
class Constraint;
enum class ConstraintType;

using json = nlohmann::json;

enum class EntityType {
    LINE_3D,
    WORKPLANE,
    LINE_2D,
    ARC_2D,
    ARC_3D,
    CIRCLE_2D,
    CIRCLE_3D,
    STEP,
    POINT_2D,
    DOCUMENT,
};

class Entity {
public:
    UUID m_uuid;
    using Type = EntityType;
    virtual Type get_type() const = 0;
    static std::string get_type_name(Type type);
    static std::string get_type_name_plural(Type type);
    static std::string get_type_name_for_n(Type type, std::size_t n);
    std::string get_type_name() const;
    std::string get_type_name_plural() const;
    std::string get_type_name_for_n(std::size_t n) const;

    template <typename... Args> bool of_type(Args &&...args) const
    {
        const auto type = get_type();
        return ((type == args) || ...);
    }

    virtual ~Entity();
    virtual json serialize() const;

    static std::unique_ptr<Entity> new_from_json(const UUID &uu, const json &j,
                                                 const std::filesystem::path &containing_dir);
    virtual std::unique_ptr<Entity> clone() const = 0;

    virtual double get_param(unsigned int point, unsigned int axis) const = 0;
    virtual void set_param(unsigned int point, unsigned int axis, double value) = 0;

    virtual glm::dvec3 get_point(unsigned int point, const Document &doc) const;
    virtual bool is_valid_point(unsigned int point) const;

    virtual void accept(EntityVisitor &visitor) const = 0;

    std::set<const Constraint *> get_constraints(const Document &doc) const;
    std::set<ConstraintType> get_constraint_types(const Document &doc) const;

    UUID m_group;
    std::string m_name;
    virtual bool has_name() const
    {
        return false;
    }

    bool m_construction = false;

    ItemKind m_kind = ItemKind::USER;
    UUID m_generated_from;
    bool m_selection_invisible = false;

    bool m_visible = true;

    bool can_delete(const Document &doc) const;
    bool can_move(const Document &doc) const;

    std::map<unsigned int, EntityAndPoint> m_move_instead;

    virtual std::set<UUID> get_referenced_entities() const;

protected:
    explicit Entity(const UUID &uu);
    explicit Entity(const UUID &uu, const json &j);
};

} // namespace dune3d

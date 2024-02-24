#pragma once
#include "util/uuid.hpp"
#include <map>
#include <memory>
#include "nlohmann/json_fwd.hpp"
#include <filesystem>
#include <set>
#include <glm/glm.hpp>
#include "util/file_version.hpp"
#include "entity/entity_and_point.hpp"

namespace dune3d {
using json = nlohmann::json;
class Entity;
class Constraint;
class Group;
class Body;
enum class GroupType;

struct ItemsToDelete {
    std::set<UUID> entities;
    std::set<UUID> groups;
    std::set<UUID> constraints;
    UUID get_first_group(const class Document &doc) const;

    void append(const ItemsToDelete &other);
    void subtract(const ItemsToDelete &other);
    bool empty() const;
    size_t size() const;
};

class Document {
public:
    Document();
    explicit Document(const json &j, const std::filesystem::path &containing_dir);
    static Document new_from_file(const std::filesystem::path &path);
    Document(const Document &other);

    std::map<UUID, std::unique_ptr<Entity>> m_entities;
    std::map<UUID, std::unique_ptr<Constraint>> m_constraints;

    FileVersion m_version;
    static unsigned int get_app_version();

    template <typename T> T &add_entity(const UUID &uu)
    {
        auto en = std::make_unique<T>(uu);
        auto p = en.get();
        m_entities.emplace(uu, std::move(en));
        return *p;
    }

    template <typename T = Entity> T &get_entity(const UUID &uu)
    {
        return dynamic_cast<T &>(*m_entities.at(uu));
    }

    template <typename T> T &get_or_add_entity(const UUID &uu, bool *was_added = nullptr)
    {
        if (m_entities.count(uu)) {
            if (was_added)
                *was_added = false;
            return dynamic_cast<T &>(*m_entities.at(uu));
        }
        else {
            if (was_added)
                *was_added = true;
            return add_entity<T>(uu);
        }
    }

    template <typename T = Entity> const T &get_entity(const UUID &uu) const
    {
        return dynamic_cast<const T &>(*m_entities.at(uu));
    }

    template <typename T = Constraint> const T &get_constraint(const UUID &uu) const
    {
        return dynamic_cast<const T &>(*m_constraints.at(uu));
    }

    template <typename T = Constraint> T &get_constraint(const UUID &uu)
    {
        return dynamic_cast<T &>(*m_constraints.at(uu));
    }

    template <typename T = Constraint> T *get_constraint_ptr(const UUID &uu)
    {
        return dynamic_cast<T *>(m_constraints.at(uu).get());
    }

    template <typename T> T &add_constraint(const UUID &uu)
    {
        auto en = std::make_unique<T>(uu);
        auto p = en.get();
        m_constraints.emplace(uu, std::move(en));
        return *p;
    }

    const auto &get_groups() const
    {
        return m_groups;
    }

    template <typename T = Group> const T &get_group(const UUID &uu) const
    {
        return dynamic_cast<const T &>(*m_groups.at(uu));
    }

    template <typename T = Group> T &get_group(const UUID &uu)
    {
        return dynamic_cast<T &>(*m_groups.at(uu));
    }

    template <typename T> T &add_group(const UUID &uu)
    {
        auto en = std::make_unique<T>(uu);
        auto p = en.get();
        m_groups.emplace(uu, std::move(en));
        return *p;
    }

    template <typename T> T &insert_group(const UUID &uu, const UUID &after)
    {
        auto en = std::make_unique<T>(uu);
        auto p = en.get();
        insert_group(std::move(en), after);
        return *p;
    }

    glm::dvec3 get_point(const EntityAndPoint &ep) const;
    bool is_valid_point(const EntityAndPoint &ep) const;

    std::vector<Group *> get_groups_sorted();
    std::vector<const Group *> get_groups_sorted() const;

    void accumulate_first_group(const Group *&first_group, const UUID &group_uu) const;

    class BodyGroups {
    public:
        BodyGroups(const Body &b) : body(b)
        {
        }
        const Body &body;
        const Group &get_group() const
        {
            return *groups.front();
        }
        std::vector<const Group *> groups;
    };
    std::vector<BodyGroups> get_groups_by_body() const;
    UUID get_group_rel(const UUID &group, int delta) const;

    void erase_invalid();
    void update_pending(const UUID &last_group = UUID(), const std::vector<EntityAndPoint> &dragged = {});

    void set_group_generate_pending(const UUID &group);
    void set_group_solve_pending(const UUID &group);
    void set_group_update_solid_model_pending(const UUID &group);

    enum class MoveGroup { UP, DOWN, END_OF_BODY, END_OF_DOCUMENT };
    UUID get_group_after(const UUID &group, MoveGroup dir) const;

    bool reorder_group(const UUID &group, const UUID &after);

    ItemsToDelete get_additional_items_to_delete(const ItemsToDelete &items) const;
    void delete_items(const ItemsToDelete &items);

    std::set<const Constraint *> find_constraints(const std::set<EntityAndPoint> &enps) const;

    std::string find_next_group_name(GroupType type) const;

    json serialize() const;

    ~Document();

private:
    std::map<UUID, std::unique_ptr<Group>> m_groups;

    UUID m_first_group_generate;
    UUID m_first_group_solve;
    UUID m_first_group_update_solid_model;

    void generate_group(Group &group);
    void solve_group(Group &group, const std::vector<EntityAndPoint> &dragged);
    void update_solid_model(Group &group);

    void update_group_if_less(UUID &uu, const UUID &new_group);

    void insert_group(std::unique_ptr<Group> group, const UUID &after);
};
} // namespace dune3d

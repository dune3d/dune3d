#pragma once
#include "util/uuid.hpp"
#include <map>
#include <memory>
#include "nlohmann/json_fwd.hpp"
#include <filesystem>
#include <set>
#include <glm/glm.hpp>
#include "util/file_version.hpp"

namespace dune3d {
using json = nlohmann::json;
class Entity;
class Constraint;
class Group;
class Body;
class EntityAndPoint;

class Document {
public:
    Document();
    explicit Document(const json &j, const std::filesystem::path &containing_dir);
    static Document new_from_file(const std::filesystem::path &path);
    Document(const Document &other);

    std::map<UUID, std::unique_ptr<Entity>> m_entities;
    std::map<UUID, std::unique_ptr<Constraint>> m_constraints;
    std::map<UUID, std::unique_ptr<Group>> m_groups;

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

    template <typename T> T &get_or_add_entity(const UUID &uu)
    {
        if (m_entities.count(uu))
            return dynamic_cast<T &>(*m_entities.at(uu));
        else
            return add_entity<T>(uu);
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

    std::vector<Group *> get_groups_sorted();
    std::vector<const Group *> get_groups_sorted() const;

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
    void generate_all();
    void solve_all();
    void update_solid_models();

    bool reorder_group(const UUID &group, const UUID &after);

    struct ItemsToDelete {
        std::set<UUID> entities;
        std::set<UUID> groups;
        std::set<UUID> constraints;

        void append(const ItemsToDelete &other);
        void subtract(const ItemsToDelete &other);
        bool empty() const;
        size_t size() const;
    };

    ItemsToDelete get_additional_items_to_delete(const ItemsToDelete &items) const;
    void delete_items(const ItemsToDelete &items);

    json serialize() const;

    ~Document();

private:
    void insert_group(std::unique_ptr<Group> group, const UUID &after);
};
} // namespace dune3d

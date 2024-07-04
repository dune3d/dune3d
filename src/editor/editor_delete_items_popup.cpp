#include "editor.hpp"
#include "dune3d_appwindow.hpp"
#include <format>
#include "util/util.hpp"
#include "document/document.hpp"
#include "document/constraint/constraint.hpp"
#include "document/entity/entity.hpp"

namespace dune3d {

static std::string make_summary(const ItemsToDelete &items, const Document &doc, const std::string &sep,
                                const std::string &prefix)
{
    std::string ex;
    if (items.entities.size() == 1) {
        ex += prefix + "one " + doc.get_entity(*items.entities.begin()).get_type_name() + " Entity";
    }
    else if (items.entities.size() > 1) {
        ex += std::format("{}{} Entities", prefix, items.entities.size());
    }

    if (items.constraints.size() == 1) {
        if (ex.size())
            ex += sep;
        ex += prefix + "one " + doc.get_constraint(*items.constraints.begin()).get_type_name() + " Constraint";
    }
    else if (items.constraints.size() > 1) {
        if (ex.size())
            ex += sep;
        ex += std::format("{}{} Constraints", prefix, items.constraints.size());
    }

    if (items.groups.size() == 1) {
        if (ex.size())
            ex += sep;
        ex += prefix + "one " + doc.get_group(*items.groups.begin()).get_type_name() + " Group";
    }
    else if (items.groups.size() > 1) {
        if (ex.size())
            ex += sep;
        ex += std::format("{}{} Groups", prefix, items.groups.size());
    }
    return ex;
}


void Editor::show_delete_items_popup(const ItemsToDelete &items_selected, const ItemsToDelete &items_all)
{
    ItemsToDelete items_extra = items_all;
    items_extra.subtract(items_selected);

    const auto &doc = m_core.get_current_document();

    // remove generated entities
    map_erase_if(items_extra.entities,
                 [&doc](const auto &uu) { return doc.get_entity(uu).m_kind == ItemKind::GENRERATED; });

    // remove constraints dependent on selected entities in same group
    map_erase_if(items_extra.constraints, [&doc, &items_selected](const auto &uu) {
        auto &constraint = doc.get_constraint(uu);
        auto deps = constraint.get_referenced_entities();
        std::set<UUID> isect;
        std::ranges::set_intersection(deps, items_selected.entities, std::inserter(isect, isect.begin()));
        for (const auto &uu : isect) {
            auto &en = doc.get_entity(uu);
            if (en.m_group == constraint.m_group)
                return true;
        }
        return false;
    });


    if (items_extra.empty())
        return;


    std::string ex = make_summary(items_selected, doc, ", ", "");

    std::map<UUID, ItemsToDelete> items_extra_by_group;
    for (const auto &uu : items_extra.entities) {
        auto &group = doc.get_entity(uu).m_group;
        items_extra_by_group[group].entities.insert(uu);
    }
    for (const auto &uu : items_extra.constraints) {
        auto &group = doc.get_constraint(uu).m_group;
        items_extra_by_group[group].constraints.insert(uu);
    }

    const auto current_group = m_core.get_current_group();

    std::string detail;
    for (auto group : doc.get_groups_sorted()) {
        if (items_extra.groups.contains(group->m_uuid)) {
            if (detail.size())
                detail += "\n";
            if (group->m_uuid == current_group)
                detail += "• Deleted current group";
            else
                detail += "• Deleted group " + group->m_name;
        }
        else if (items_extra_by_group.contains(group->m_uuid)) {
            if (detail.size())
                detail += "\n";
            if (group->m_uuid == current_group)
                detail += "• In current group";
            else
                detail += "• In group " + group->m_name;

            const auto &group_items = items_extra_by_group.at(group->m_uuid);
            std::map<Entity::Type, unsigned int> entities;
            std::map<Constraint::Type, unsigned int> constraints;
            for (const auto &uu : group_items.entities) {
                entities[doc.get_entity(uu).get_type()]++;
            }
            for (const auto &uu : group_items.constraints) {
                constraints[doc.get_constraint(uu).get_type()]++;
            }
            for (const auto &[type, count] : entities) {
                if (count == 1) {
                    detail += std::format("\n  • One {} Entity", Entity::get_type_name(type));
                }
                else {
                    detail += std::format("\n  • {} {} Entities", count, Entity::get_type_name(type));
                }
            }
            for (const auto &[type, count] : constraints) {
                if (count == 1) {
                    detail += std::format("\n  • One {} Constraint", Constraint::get_type_name(type));
                }
                else {
                    detail += std::format("\n  • {} {} Constraints", count, Constraint::get_type_name(type));
                }
            }
        }
    }

    m_win.show_delete_items_popup("Deleting " + ex + " also deleted", make_summary(items_extra, doc, "\n", "• "),
                                  detail);
}

} // namespace dune3d

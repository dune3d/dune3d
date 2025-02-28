#include "tool_create_coincident_constraints.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/ientity_in_workplane.hpp"
#include "document/constraint/constraint_points_coincident.hpp"

#include "editor/editor_interface.hpp"
#include "util/action_label.hpp"
#include "tool_common_impl.hpp"
#include "dialogs/enter_datum_window.hpp"
#include "dialogs/dialogs.hpp"

#include <format>

namespace dune3d {

std::set<Entity *> ToolCreateCoincidentConstraints::get_entities()
{
    UUID wrkpl;
    std::set<Entity *> r;
    for (const auto &sr : m_selection) {
        if (!sr.is_entity())
            continue;
        auto &en = get_entity(sr.item);
        if (!en.of_type(Entity::Type::ARC_2D, Entity::Type::LINE_2D, Entity::Type::BEZIER_2D))
            continue;
        const auto &en_wrkpl = dynamic_cast<const IEntityInWorkplane &>(en);
        if (!wrkpl)
            wrkpl = en_wrkpl.get_workplane();
        else if (wrkpl != en_wrkpl.get_workplane())
            return {};
        r.insert(&en);
    }
    return r;
}

ToolBase::CanBegin ToolCreateCoincidentConstraints::can_begin()
{
    return get_entities().size() > 1;
}

struct Node {
    EntityAndPoint enp;
    glm::dvec2 pos;
    unsigned int tag = 0;
};

using Buckets = std::map<std::pair<int64_t, int64_t>, std::vector<Node>>;

static void update_tag(Buckets &buckets, unsigned int old_tag, unsigned int new_tag)
{
    for (auto &[k, nodes] : buckets) {
        for (auto &node : nodes) {
            if (node.tag == old_tag)
                node.tag = new_tag;
        }
    }
}

void ToolCreateCoincidentConstraints::reset()
{
    auto &doc = get_doc();
    for (const auto &uu : m_constraints) {
        doc.m_constraints.erase(uu);
    }
    m_constraints.clear();

    m_entities_delete.clear();
    auto &last_doc = m_core.get_current_last_document();
    for (auto en : m_entities) {
        auto &last_en = last_doc.get_entity(en->m_uuid);
        for (unsigned int pt = 1; pt <= 4; pt++) {
            if (!en->is_valid_point(pt))
                continue;
            en->set_param(pt, 0, last_en.get_param(pt, 0));
            en->set_param(pt, 1, last_en.get_param(pt, 1));
            en->m_visible = true;
        }
    }
}

void ToolCreateCoincidentConstraints::apply()
{
    Buckets buckets;
    for (auto en : m_entities) {
        for (unsigned int pt = 1; pt <= 4; pt++) {
            if (!en->is_valid_point(pt))
                continue;
            const auto p = dynamic_cast<const IEntityInWorkplane &>(*en).get_point_in_workplane(pt);
            const int64_t x = round(p.x / m_tolerance);
            const int64_t y = round(p.y / m_tolerance);
            const auto key = std::make_pair(x, y);
            buckets[key].emplace_back(EntityAndPoint{en->m_uuid, pt}, p);
        }
    }
    {
        unsigned int tag = 1;
        for (auto &[key, nodes] : buckets) {
            const auto [x, y] = key;
            std::vector<Node *> all_nodes;
            all_nodes.reserve(nodes.size() * 9);
            for (auto &node : nodes) {
                all_nodes.push_back(&node);
            }

            // look at all neighboring nodes
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx == 0 && dy == 0) // this node
                        continue;
                    auto other_it = buckets.find(std::make_pair(x + dx, y + dy));
                    if (other_it == buckets.end())
                        continue;
                    auto &other_nodes = other_it->second;
                    for (auto &node : other_nodes) {
                        all_nodes.push_back(&node);
                    }
                }
            }

            // now find nodes that are closer together than m_tolerance
            for (size_t i = 0; i < all_nodes.size(); i++) {
                auto na = all_nodes.at(i);
                for (size_t j = i + 1; j < all_nodes.size(); j++) {
                    auto nb = all_nodes.at(j);
                    auto distance = glm::length(na->pos - nb->pos);
                    if (distance >= m_tolerance)
                        continue;
                    // nodes are within tolerance

                    if (na->tag == 0 && nb->tag == 0) {
                        na->tag = tag;
                        nb->tag = tag;
                        tag++;
                    }
                    else if (na->tag == 0 && nb->tag != 0) {
                        na->tag = nb->tag;
                    }
                    else if (na->tag != 0 && nb->tag == 0) {
                        nb->tag = na->tag;
                    }
                    else if (na->tag != 0 && nb->tag != 0) {
                        // both nodes are already part of a cluster, merge them
                        if (na->tag != nb->tag)
                            update_tag(buckets, na->tag, nb->tag);
                        assert(na->tag == nb->tag);
                    }
                    else {
                        throw std::runtime_error("how did we get here?");
                    }
                }
            }
        }
    }
    std::map<EntityAndPoint, unsigned int> enp_tags;

    for (auto &[k, nodes] : buckets) {
        for (auto &node : nodes) {
            if (node.tag != 0)
                enp_tags.emplace(node.enp, node.tag);
        }
    }

    for (auto en : m_entities) {
        std::set<unsigned int> tags;
        bool has_tag0 = false;
        for (unsigned int pt = 1; pt <= 4; pt++) {
            if (!en->is_valid_point(pt))
                continue;
            auto enp_tag_it = enp_tags.find(EntityAndPoint{en->m_uuid, pt});
            if (enp_tag_it == enp_tags.end()) {
                has_tag0 = true;
                break;
            }
            auto enp_tag = enp_tag_it->second;
            tags.insert(enp_tag);
        }
        if (!has_tag0 && tags.size() == 1) {
            m_entities_delete.insert(en->m_uuid);
            en->m_visible = false;
        }
    }

    std::map<unsigned int, EntityAndPoint> tag_points;
    for (auto &[k, nodes] : buckets) {
        for (auto &node : nodes) {
            if (node.tag == 0)
                continue;
            if (m_entities_delete.contains(node.enp.entity))
                continue;
            auto it = tag_points.find(node.tag);
            if (it == tag_points.end()) {
                tag_points.emplace(node.tag, node.enp);
            }
            else {
                auto other_enp = it->second;
                if (m_entities_delete.contains(other_enp.entity))
                    continue;
                auto &constraint = add_constraint<ConstraintPointsCoincident>();
                constraint.m_entity1 = node.enp;
                constraint.m_entity2 = other_enp;
                constraint.m_wrkpl = m_wrkpl;
                m_constraints.push_back(constraint.m_uuid);
            }
        }
    }

    m_core.solve_current();

    update_tip();
}

void ToolCreateCoincidentConstraints::update_tip()
{
    std::string tip =
            std::format("Created {} Constraint{}", m_constraints.size(), m_constraints.size() != 1 ? "s" : "");
    if (m_entities_delete.size()) {
        tip += std::format(", will delete {} zero-sized {}", m_entities_delete.size(),
                           Entity::get_type_name_for_n(EntityType::INVALID, m_entities_delete.size()));
    }

    m_intf.tool_bar_set_tool_tip(tip);

    std::string tol = "tolerance: ";
    if (m_tolerance >= 0.1)
        tol += std::format("{:.3g}mm", m_tolerance);
    else
        tol += std::format("{:.3g}um", m_tolerance * 1e3);

    {
        std::vector<ActionLabelInfo> actions;
        actions.emplace_back(InToolActionID::RMB, "finish");
        actions.emplace_back(InToolActionID::CANCEL, "cancel");
        actions.emplace_back(InToolActionID::ENTER_TOLERANCE);

        if (!m_win)
            actions.emplace_back(InToolActionID::TOLERANCE_INC, InToolActionID::TOLERANCE_DEC, tol);
        m_intf.tool_bar_set_actions(actions);
    }
}

ToolResponse ToolCreateCoincidentConstraints::begin(const ToolArgs &args)
{
    m_entities = get_entities();
    m_wrkpl = dynamic_cast<const IEntityInWorkplane &>(**m_entities.begin()).get_workplane();

    apply();


    m_intf.set_no_canvas_update(true);
    m_intf.canvas_update_from_tool();

    return ToolResponse();
}

ToolResponse ToolCreateCoincidentConstraints::commit()
{
    ItemsToDelete items_delete;
    items_delete.entities = m_entities_delete;
    ItemsToDelete items_to_delete = items_delete;

    auto extra_items = get_doc().get_additional_items_to_delete(items_to_delete);
    items_to_delete.append(extra_items);

    m_intf.show_delete_items_popup(items_delete, items_to_delete);

    get_doc().delete_items(items_to_delete);
    return ToolResponse::commit();
}

ToolResponse ToolCreateCoincidentConstraints::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::RMB:
            return commit();

        case InToolActionID::TOLERANCE_DEC:
        case InToolActionID::TOLERANCE_INC:
            if (!m_win) {
                if (args.action == InToolActionID::TOLERANCE_INC)
                    m_tolerance *= 10;
                else
                    m_tolerance /= 10;
                reset();
                apply();
                m_intf.canvas_update_from_tool();
            }
            break;

        case InToolActionID::ENTER_TOLERANCE:
            m_last_tolerance = m_tolerance;
            m_win = m_intf.get_dialogs().show_enter_datum_window("Enter sides", DatumUnit::MM, m_tolerance);
            m_win->set_range(0, 1000);
            m_win->set_step_size(1e-3);
            break;


        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
    }
    else if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::UPDATE) {
                if (auto d = dynamic_cast<const ToolDataEnterDatumWindow *>(args.data.get())) {
                    m_tolerance = d->value;
                    reset();
                    apply();
                    m_intf.canvas_update_from_tool();
                }
            }
            else if (data->event == ToolDataWindow::Event::OK) {
                m_win->close();
                m_win = nullptr;
                return commit();
            }
            else if (data->event == ToolDataWindow::Event::CLOSE) {
                if (m_win) {
                    m_tolerance = m_last_tolerance;
                    m_win = nullptr;
                    reset();
                    apply();
                }
            }
        }
    }

    return ToolResponse();
}

} // namespace dune3d

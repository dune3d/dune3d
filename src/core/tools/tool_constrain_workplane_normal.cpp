#include "tool_constrain_workplane_normal.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/constraint/constraint_workplane_normal.hpp"
#include "core/tool_id.hpp"
#include <optional>
#include <iostream>
#include "util/selection_util.hpp"
#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"


namespace dune3d {

static std::optional<UUID> one_wrkpl_from_selection(const Document &doc, const std::set<SelectableRef> &sel)
{
    if (sel.size() != 1)
        return {};
    auto it = sel.begin();
    auto &sr1 = *it++;

    if (sr1.type != SelectableRef::Type::ENTITY)
        return {};

    auto &en1 = doc.get_entity(sr1.item);
    if (en1.get_type() == Entity::Type::WORKPLANE)
        return {en1.m_uuid};

    return {};
}

bool ToolConstrainWorkplaneNormal::can_begin()
{
    return one_wrkpl_from_selection(get_doc(), m_selection).has_value();
}

ToolResponse ToolConstrainWorkplaneNormal::begin(const ToolArgs &args)
{
    auto tp = one_wrkpl_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();

    m_wrkpl = &get_doc().get_entity<EntityWorkplane>(*tp);

    if (m_wrkpl->m_group != m_core.get_current_group())
        return ToolResponse::end();

    m_intf.enable_hover_selection();


    return ToolResponse();
}

ToolResponse ToolConstrainWorkplaneNormal::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            auto hsel = m_intf.get_hover_selection();

            if (!hsel.has_value())
                return ToolResponse();

            if (hsel->type != SelectableRef::Type::ENTITY)
                return ToolResponse();

            if (hsel->point != 0)
                return ToolResponse();

            auto &entity = get_entity(hsel->item);
            if (entity.get_type() != Entity::Type::LINE_2D && entity.get_type() != Entity::Type::LINE_3D)
                return ToolResponse();

            if (entity.m_group == m_core.get_current_group())
                return ToolResponse();

            if (m_line1 == nullptr) {
                m_line1 = &entity;
            }
            else {
                auto &constraint = add_constraint<ConstraintWorkplaneNormal>();
                constraint.m_line1 = m_line1->m_uuid;
                constraint.m_line2 = entity.m_uuid;
                constraint.m_wrkpl = m_wrkpl->m_uuid;
                auto uvn = constraint.get_uvn(get_doc());
                if (!uvn.has_value())
                    return ToolResponse::revert();

                auto d = glm::dot(glm::vec3(uvn->n), m_intf.get_cam_normal());
                if (d > 0)
                    constraint.m_flip_normal = true;

                return ToolResponse::commit();
            }

            m_selection.insert(*hsel);

        } break;

        case InToolActionID::RMB:


        case InToolActionID::CANCEL:

            return ToolResponse::revert();

        default:;
        }
    }

    return ToolResponse();
}
} // namespace dune3d

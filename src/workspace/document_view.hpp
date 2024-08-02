#pragma once
#include "util/uuid.hpp"
#include "idocument_view.hpp"
#include "entity_view.hpp"
#include "nlohmann/json_fwd.hpp"
#include <map>

namespace dune3d {

using json = nlohmann::json;

class DocumentView : public IDocumentView {
public:
    DocumentView();
    DocumentView(const DocumentView &other);
    explicit DocumentView(const json &j);
    // only used for persistence
    UUID m_current_group;

    class GroupView {
    public:
        GroupView();
        GroupView(const json &j);
        bool m_visible = true;
        json serialize() const;
    };
    std::map<UUID, GroupView> m_group_views;

    class BodyView {
    public:
        BodyView();
        BodyView(const json &j);
        bool m_visible = true;
        bool m_solid_model_visible = true;
        json serialize() const;
    };
    std::map<UUID, BodyView> m_body_views;

    std::map<UUID, std::unique_ptr<EntityView>> m_entity_views;

    bool body_is_visible(const UUID &uu) const override;
    bool body_solid_model_is_visible(const UUID &uu) const override;
    bool group_is_visible(const UUID &uu) const override;

    bool m_document_is_visible = false;
    bool document_is_visible() const override
    {
        return m_document_is_visible;
    }

    bool m_show_construction_entities_from_previous_groups = false;
    bool construction_entities_from_previous_groups_are_visible() const override
    {
        return m_show_construction_entities_from_previous_groups;
    }

    json serialize() const;

    const EntityView *get_entity_view(const UUID &uu) const override;
    EntityView *get_or_create_entity_view(const UUID &en, EntityType type);
};

} // namespace dune3d

#pragma once
#include "util/uuid.hpp"
#include "workspace/idocument_view.hpp"
#include "workspace/entity_view.hpp"
#include <map>

namespace dune3d {

class DocumentView : public IDocumentView {
public:
    // only used for persistence
    UUID m_current_group;

    class GroupView {
    public:
        bool m_visible = true;
    };
    std::map<UUID, GroupView> m_group_views;

    class BodyView {
    public:
        bool m_visible = true;
        bool m_solid_model_visible = true;
    };
    std::map<UUID, BodyView> m_body_views;

    std::map<UUID, std::unique_ptr<EntityView>> m_entity_views;

    bool body_is_visible(const UUID &uu) const override;
    bool body_solid_model_is_visible(const UUID &uu) const override;
    bool group_is_visible(const UUID &uu) const override;

    bool m_document_is_visible = true;
    bool document_is_visible() const override
    {
        return m_document_is_visible;
    }

    const EntityView *get_entity_view(const UUID &uu) const override;
    EntityView *get_or_create_entity_view(const UUID &en, EntityType type);
};

} // namespace dune3d

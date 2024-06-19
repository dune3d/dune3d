#pragma once
#include "util/uuid.hpp"
#include "idocument_view.hpp"
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

    bool body_is_visible(const UUID &uu) const override;
    bool body_solid_model_is_visible(const UUID &uu) const override;
    bool group_is_visible(const UUID &uu) const override;

    bool m_document_is_visible = true;
    bool document_is_visible() const override
    {
        return m_document_is_visible;
    }
};

} // namespace dune3d

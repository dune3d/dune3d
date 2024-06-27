#include "document_view.hpp"
#include "workspace/entity_view.hpp"
#include "document/entity/entity.hpp"

namespace dune3d {
bool DocumentView::body_is_visible(const UUID &uu) const
{
    if (m_body_views.contains(uu))
        return m_body_views.at(uu).m_visible;
    return true;
}

bool DocumentView::body_solid_model_is_visible(const UUID &uu) const
{
    if (m_body_views.contains(uu))
        return m_body_views.at(uu).m_solid_model_visible;
    return true;
}

bool DocumentView::group_is_visible(const UUID &uu) const
{
    if (m_group_views.contains(uu))
        return m_group_views.at(uu).m_visible;
    return true;
}

const EntityView *DocumentView::get_entity_view(const UUID &uu) const
{
    if (m_entity_views.contains(uu))
        return m_entity_views.at(uu).get();
    return nullptr;
}

EntityView *DocumentView::get_or_create_entity_view(const UUID &en, EntityType type)
{
    if (m_entity_views.contains(en))
        return m_entity_views.at(en).get();

    auto view = EntityView::create_for_type(type);
    if (!view)
        return nullptr;
    auto viewp = view.get();

    m_entity_views.emplace(en, std::move(view));
    return viewp;
}

} // namespace dune3d

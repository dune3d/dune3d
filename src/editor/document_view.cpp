#include "document_view.hpp"

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
} // namespace dune3d

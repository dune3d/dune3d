#include "document_view.hpp"
#include "document/entity/entity.hpp"
#include "nlohmann/json.hpp"

namespace dune3d {


DocumentView::DocumentView() = default;
DocumentView::DocumentView(const DocumentView &other)
    : m_current_group(other.m_current_group), m_group_views(other.m_group_views), m_body_views(other.m_body_views),
      m_document_is_visible(other.m_document_is_visible),
      m_show_construction_entities_from_previous_groups(other.m_show_construction_entities_from_previous_groups)
{
    for (const auto &[uu, it] : other.m_entity_views) {
        m_entity_views.emplace(uu, it->clone());
    }
}

bool DocumentView::body_is_visible(const UUID &uu) const
{
    if (m_body_views.contains(uu))
        return m_body_views.at(uu).m_visible;
    return true;
}

bool DocumentView::body_is_expanded(const UUID &uu) const
{
    if (m_body_views.contains(uu))
        return m_body_views.at(uu).m_expanded;
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

DocumentView::GroupView::GroupView() = default;
DocumentView::GroupView::GroupView(const json &j) : m_visible(j.at("visible").get<bool>())
{
}

json DocumentView::GroupView::serialize() const
{
    return {{"visible", m_visible}};
}

DocumentView::BodyView::BodyView() = default;
DocumentView::BodyView::BodyView(const json &j)
    : m_visible(j.at("visible").get<bool>()), m_solid_model_visible(j.at("solid_model_visible").get<bool>()),
      m_expanded(j.value("expanded", true))
{
}

json DocumentView::BodyView::serialize() const
{
    return {{"visible", m_visible}, {"solid_model_visible", m_solid_model_visible}, {"expanded", m_expanded}};
}

json DocumentView::serialize() const
{
    json j;
    j["current_group"] = m_current_group;
    j["show_construction_entities_from_previous_groups"] = m_show_construction_entities_from_previous_groups;
    {
        json o = json::object();
        for (const auto &[uu, it] : m_group_views) {
            o[uu] = it.serialize();
        }
        j["group_views"] = o;
    }
    {
        json o = json::object();
        for (const auto &[uu, it] : m_body_views) {
            o[uu] = it.serialize();
        }
        j["body_views"] = o;
    }
    {
        json o = json::object();
        for (const auto &[uu, it] : m_entity_views) {
            o[uu] = it->serialize();
        }
        j["entity_views"] = o;
    }


    return j;
}

DocumentView::DocumentView(const json &j)
    : m_current_group(j.at("current_group").get<std::string>()), m_document_is_visible(true),
      m_show_construction_entities_from_previous_groups(
              j.value("show_construction_entities_from_previous_groups", false))
{
    for (const auto &[uu, it] : j.at("group_views").items()) {
        m_group_views.emplace(uu, it);
    }
    for (const auto &[uu, it] : j.at("body_views").items()) {
        m_body_views.emplace(uu, it);
    }
    for (const auto &[uu, it] : j.at("entity_views").items()) {
        m_entity_views.emplace(uu, EntityView::new_from_json(it));
    }
}

} // namespace dune3d

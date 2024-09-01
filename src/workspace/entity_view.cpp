#include "entity_view.hpp"
#include "document/entity/entity.hpp"
#include "nlohmann/json.hpp"

namespace dune3d {
EntityView::~EntityView() = default;

std::unique_ptr<EntityView> EntityView::create_for_type(EntityType type)
{
    switch (type) {
    case EntityType::STEP:
        return std::make_unique<EntityViewSTEP>();
    default:
        return nullptr;
    }
}


std::unique_ptr<EntityView> EntityView::new_from_json(const json &j)
{
    const auto type = j.at("type").get<EntityType>();
    switch (type) {
    case EntityType::STEP:
        return std::make_unique<EntityViewSTEP>(j);
    default:
        return nullptr;
    }
}


json EntityView::serialize() const
{
    return {{"type", Entity::serialize_type(get_type())}};
}

EntityType EntityViewSTEP::get_type() const
{
    return EntityType::STEP;
}


std::unique_ptr<EntityView> EntityViewSTEP::clone() const
{
    return std::make_unique<EntityViewSTEP>(*this);
}

NLOHMANN_JSON_SERIALIZE_ENUM(EntityViewSTEP::Display,
                             {
                                     {EntityViewSTEP::Display::OFF, "off"},
                                     {EntityViewSTEP::Display::SOLID, "solid"},
                                     {EntityViewSTEP::Display::WIREFRAME, "wireframe"},
                                     {EntityViewSTEP::Display::SOLID_WIREFRAME, "solid_wireframe"},
                             })

EntityViewSTEP::EntityViewSTEP() = default;
EntityViewSTEP::EntityViewSTEP(const json &j) : m_display(j.at("display").get<Display>())
{
}

json EntityViewSTEP::serialize() const
{
    auto j = EntityView::serialize();
    j["display"] = m_display;
    return j;
}

} // namespace dune3d

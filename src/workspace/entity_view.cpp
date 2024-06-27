#include "entity_view.hpp"
#include "document/entity/entity.hpp"

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

EntityType EntityViewSTEP::get_type() const
{
    return EntityType::STEP;
}

} // namespace dune3d

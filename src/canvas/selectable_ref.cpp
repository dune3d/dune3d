#include "selectable_ref.hpp"

namespace dune3d {
EntityAndPoint SelectableRef::get_entity_and_point() const
{
    if (type != Type::ENTITY)
        throw std::runtime_error("not an entity");
    return EntityAndPoint{item, point};
}
} // namespace dune3d

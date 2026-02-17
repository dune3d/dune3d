#include "selectable_group.hpp"
#include "nlohmann/json.hpp"
#include "util/util.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"

namespace dune3d {
GroupLocalOperation::GroupLocalOperation(const UUID &uu) : Group(uu)
{
}

GroupLocalOperation::GroupLocalOperation(const UUID &uu, const json &j)
{
}

} // namespace dune3d

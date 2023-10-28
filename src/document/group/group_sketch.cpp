#include "group_sketch.hpp"
#include "nlohmann/json.hpp"
namespace dune3d {
GroupSketch::GroupSketch(const UUID &uu) : Group(uu)
{
}

GroupSketch::GroupSketch(const UUID &uu, const json &j) : Group(uu, j)
{
}

json GroupSketch::serialize() const
{
    return Group::serialize();
}

std::unique_ptr<Group> GroupSketch::clone() const
{
    return std::make_unique<GroupSketch>(*this);
}

} // namespace dune3d

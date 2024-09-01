#include "group_sketch.hpp"
#include "nlohmann/json.hpp"
#include "document/solid_model.hpp"

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

const SolidModel *GroupSketch::get_solid_model() const
{
    return m_solid_model.get();
}

void GroupSketch::update_solid_model(const Document &doc)
{
    m_solid_model = SolidModel::create(doc, *this);
}

} // namespace dune3d

#include "group_chamfer.hpp"
#include "document/solid_model/solid_model.hpp"
#include "nlohmann/json.hpp"

namespace dune3d {

GroupChamfer::GroupChamfer(const UUID &uu) : GroupLocalOperation(uu)
{
}

GroupChamfer::GroupChamfer(const UUID &uu, const json &j) : GroupLocalOperation(uu, j)
{
    if (j.contains("radius2"))
        m_radius2 = j.at("radius2").get<double>();
}

json GroupChamfer::serialize() const
{
    auto j = GroupLocalOperation::serialize();
    if (m_radius2.has_value())
        j["radius2"] = *m_radius2;

    return j;
}


std::unique_ptr<Group> GroupChamfer::clone() const
{
    return std::make_unique<GroupChamfer>(*this);
}

void GroupChamfer::update_solid_model(const Document &doc)
{
    m_solid_model = SolidModel::create(doc, *this);
}


} // namespace dune3d

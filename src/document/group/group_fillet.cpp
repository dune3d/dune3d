#include "group_fillet.hpp"
#include "document/solid_model/solid_model.hpp"

namespace dune3d {


std::unique_ptr<Group> GroupFillet::clone() const
{
    return std::make_unique<GroupFillet>(*this);
}

void GroupFillet::update_solid_model(const Document &doc)
{
    m_solid_model = SolidModel::create(doc, *this);
}


} // namespace dune3d

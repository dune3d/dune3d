#include "group_chamfer.hpp"
#include "nlohmann/json.hpp"
#include "util/util.hpp"
#include "document.hpp"
#include "entity_workplane.hpp"
#include "solid_model.hpp"

namespace dune3d {


std::unique_ptr<Group> GroupChamfer::clone() const
{
    return std::make_unique<GroupChamfer>(*this);
}

void GroupChamfer::update_solid_model(const Document &doc)
{
    m_solid_model = SolidModel::create(doc, *this);
}


} // namespace dune3d

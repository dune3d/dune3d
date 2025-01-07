#include "igroup_solid_model.hpp"
#include "group.hpp"

namespace dune3d {
const SolidModel *IGroupSolidModel::try_get_solid_model(const Group &group)
{
    auto solid = dynamic_cast<const IGroupSolidModel *>(&group);
    if (!solid)
        return nullptr;
    return solid->get_solid_model();
}

} // namespace dune3d

#include "tool_common_constrain.hpp"
#include "tool_common_impl.hpp"
#include "document/document.hpp"
#include "document/constraint/constraint.hpp"
#include "document/constraint/iconstraint_workplane.hpp"

namespace dune3d {

template <typename... Args>
bool ToolCommonConstrain::has_constraint_of_type_in_workplane(const std::set<EntityAndPoint> &enps, Args... types)
{
    auto wrkpl = get_workplane_uuid();

    auto constraints = get_doc().find_constraints(enps);
    for (auto constraint : constraints) {
        if (constraint->of_type(types...)) {
            auto constraint_wrkpl = dynamic_cast<const IConstraintWorkplane *>(constraint);

            if (!constraint_wrkpl)
                return true;

            if (constraint_wrkpl->get_workplane(get_doc()) == wrkpl)
                return true;
        }
    }

    return false;
}

template <typename... Args>
bool ToolCommonConstrain::has_constraint_of_type(const std::set<EntityAndPoint> &enps, Args... types)
{
    auto constraints = get_doc().find_constraints(enps);
    for (auto constraint : constraints) {
        if (constraint->of_type(types...))
            return true;
    }
    return false;
}

} // namespace dune3d

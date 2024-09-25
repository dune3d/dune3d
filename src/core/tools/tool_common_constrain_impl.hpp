#include "tool_common_constrain.hpp"
#include "tool_common_impl.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
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

[[maybe_unused]]
static bool entity_from_current_group(const Document &doc, const UUID &current_group, const Entity &en)
{
    return en.m_group == current_group;
}

[[maybe_unused]]
static bool entity_from_current_group(const Document &doc, const UUID &current_group, const UUID &entity)
{
    return entity_from_current_group(doc, current_group, doc.get_entity(entity));
}

[[maybe_unused]]
static bool entity_from_current_group(const Document &doc, const UUID &current_group, const EntityAndPoint &enp)
{
    return entity_from_current_group(doc, current_group, enp.entity);
}

template <typename... Args> bool ToolCommonConstrain::any_entity_from_current_group(const Args &...enps)
{
    const auto current_group = m_core.get_current_group();
    auto &doc = get_doc();
    return (entity_from_current_group(doc, current_group, enps) || ...);
}

template <typename... Args> bool ToolCommonConstrain::any_entity_from_current_group(const std::tuple<Args...> &enps)
{
    return any_entity_from_current_group_helper(
            enps, std::make_index_sequence<std::tuple_size_v<std::decay_t<decltype(enps)>>>{});
}

template <typename... Args, size_t... Is>
bool ToolCommonConstrain::any_entity_from_current_group_helper(const std::tuple<Args...> &enps,
                                                               std::index_sequence<Is...>)
{
    return any_entity_from_current_group(std::get<Is>(enps)...);
}


} // namespace dune3d

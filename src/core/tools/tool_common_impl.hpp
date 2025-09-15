#pragma once
#include "tool_common.hpp"
#include "document/document.hpp"

namespace dune3d {

template <typename T> T &ToolCommon::add_entity(const UUID &uu)
{
    if (!can_create_entity())
        throw std::runtime_error("can't create entity in this group");
    set_current_group_generate_pending();
    auto &en = m_core.get_current_document().add_entity<T>(uu);
    en.m_group = m_core.get_current_group();
    return en;
}

template <typename T> T &ToolCommon::get_entity(const UUID &uu)
{
    return m_core.get_current_document().get_entity<T>(uu);
}

template <typename T> T &ToolCommon::add_constraint(const UUID &uu)
{
    if (!can_create_constraint())
        throw std::runtime_error("can't create constraint in this group");
    set_current_group_solve_pending();
    return just_add_constraint<T>(uu);
}

template <typename T> T &ToolCommon::just_add_constraint(const UUID &uu)
{
    if (!can_create_constraint())
        throw std::runtime_error("can't create constraint in this group");
    auto &co = m_core.get_current_document().add_constraint<T>(uu);
    co.m_group = m_core.get_current_group();
    return co;
}
} // namespace dune3d

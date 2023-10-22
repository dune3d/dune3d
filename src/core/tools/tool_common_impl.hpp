#pragma once
#include "tool_common.hpp"
#include "document/document.hpp"

namespace dune3d {

template <typename T> T &ToolCommon::add_entity(const UUID &uu)
{
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
    auto &co = m_core.get_current_document().add_constraint<T>(uu);
    co.m_group = m_core.get_current_group();
    return co;
}
} // namespace dune3d

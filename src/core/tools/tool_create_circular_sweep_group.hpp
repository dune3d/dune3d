#pragma once
#include "tool_common.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include "document/group/group.hpp"

namespace dune3d {

class ToolCreateCircularSweepGroup : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool is_specific() override
    {
        return false;
    }

    CanBegin can_begin() override;

    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
        };
    }

private:
    bool is_valid_axis_selection(const SelectableRef &sel);
    ToolResponse create_group(const UUID &axis_uu);
    Group::Type get_group_type() const;

    UUID m_source_group;
    UUID m_wrkpl;
    bool m_with_body = false;
};
} // namespace dune3d

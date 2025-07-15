#pragma once
#include "icore.hpp"
#include "canvas/selectable_ref.hpp"
#include <memory>
#include "tool_data.hpp"
#include "bitmask_operators.hpp"
#include <glm/glm.hpp>
#include <set>
#include <optional>

namespace dune3d {

class ICore;
class EditorInterface;

enum class ToolEventType { NONE, MOVE, ACTION, DATA };

enum class ToolID;
enum class InToolActionID;

/**
 * This is what a Tool receives when the user did something.
 * i.e. moved the cursor or pressed key
 */
class ToolArgs {
public:
    ToolEventType type = ToolEventType::NONE;
    std::set<SelectableRef> selection;
    bool m_keep_selection = false;
    InToolActionID action;
    std::unique_ptr<ToolData> data = nullptr;
    ToolID tool_id;

    /*Target target;
    int work_layer = 0;
    std::unique_ptr<ToolData> data = nullptr;*/
    ToolArgs();
};

/**
 * To signal back to the core what the Tool did, a Tool returns a ToolResponse.
 */
class ToolResponse {
public:
    // ToolID next_tool;
    //  std::unique_ptr<ToolData> data = nullptr;
    enum class Result { NOP, END, COMMIT, REVERT };
    Result result = Result::NOP;
    /**
     * Use this if you're done. The Core will then delete the active tool and
     * initiate a rebuild.
     */
    static ToolResponse end()
    {
        return ToolResponse(Result::END);
    }

    static ToolResponse commit()
    {
        return ToolResponse(Result::COMMIT);
    }

    static ToolResponse revert()
    {
        return ToolResponse(Result::REVERT);
    }

    /**
     * If you want another Tool to be launched you've finished, use this one.
     * Make sure to set `tool_args.tool_id`
     */
    static ToolResponse next(Result res, ToolArgs tool_args)
    {
        ToolResponse r(res);
        r.next_tool_args = std::move(tool_args);
        return r;
    };

    ToolResponse();

    std::optional<ToolArgs> next_tool_args;

private:
    ToolResponse(Result r);
};

/**
 * Common interface for all Tools
 */
class ToolBase {
public:
    enum class Flags { DEFAULT = 0, TRANSIENT = (1 << 0), PREVIEW = (1 << 1) };

    ToolBase(ToolID tool_id, ICore &core, EditorInterface &intf, Flags flags);

    virtual void apply_settings()
    {
    }

    virtual std::set<InToolActionID> get_actions() const
    {
        return {};
    }

    /**
     * Gets called right after the constructor has finished.
     * Used to get the initial placement right and set things up.
     * For non-interactive Tools (e.g. DELETE), this one may return
     * ToolResponse::end()
     */
    virtual ToolResponse begin(const ToolArgs &args) = 0;

    /**
     * Gets called whenever the user generated some sort of input.
     */
    virtual ToolResponse update(const ToolArgs &args) = 0;

    /**
     * @returns if this Tool can begin in sensible way
     */

    struct CanBegin {
        enum class E {
            NO,          // can't begin at all
            YES,         // can begin
            YES_NO_MENU, // can begin, but hide from context menu
        };
        using enum E;

        const E can_begin;
        CanBegin(E e) : can_begin(e)
        {
        }
        CanBegin(bool x) : can_begin(x ? YES : NO)
        {
        }
        operator E() const
        {
            return can_begin;
        }
    };

    virtual CanBegin can_begin()
    {
        return true;
    }

    /**
     * @returns true if this Tool is specific to the selection
     */
    virtual bool is_specific()
    {
        return false;
    }

    virtual bool needs_delayed_begin() const
    {
        return false;
    }

    std::set<SelectableRef> m_selection;

    virtual ~ToolBase()
    {
    }

    ToolID get_id() const
    {
        return m_tool_id;
    }

protected:
    /*virtual ToolSettings *get_settings()
    {
        return nullptr;
    }*/

    const ToolID m_tool_id;
    ICore &m_core;
    EditorInterface &m_intf;
    const bool m_is_transient;
    const bool m_is_preview;
};


} // namespace dune3d


template <> struct enable_bitmask_operators<dune3d::ToolBase::Flags> {
    static constexpr bool enable = true;
};

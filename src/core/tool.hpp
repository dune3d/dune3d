#pragma once
#include "icore.hpp"
#include "canvas/selectable_ref.hpp"
#include <memory>
#include "tool_data.hpp"
#include "bitmask_operators.hpp"
#include <glm/glm.hpp>
#include <set>

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
     */
    /*static ToolResponse next(Result res, ToolID t, std::unique_ptr<ToolData> data = nullptr)
    {
        ToolResponse r(res);
        r.next_tool = t;
        r.data = std::move(data);
        return r;
    };*/

    ToolResponse();

private:
    ToolResponse(Result r);
};

/**
 * Common interface for all Tools
 */
class ToolBase {
public:
    enum class Flags { DEFAULT = 0, TRANSIENT = (1 << 0) };

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
     * @returns true if this Tool can begin in sensible way
     */
    virtual bool can_begin()
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
};


} // namespace dune3d


template <> struct enable_bitmask_operators<dune3d::ToolBase::Flags> {
    static constexpr bool enable = true;
};

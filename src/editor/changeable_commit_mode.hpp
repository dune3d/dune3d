#pragma once
#include <sigc++/sigc++.h>

namespace dune3d {

class ChangeableCommitMode {
public:
    enum class CommitMode { IMMEDIATE, DELAYED, EXECUTE_DELAYED };

    typedef sigc::signal<void(CommitMode)> type_signal_changed;
    type_signal_changed signal_changed()
    {
        return m_signal_changed;
    }

    type_signal_changed m_signal_changed;
};
} // namespace dune3d

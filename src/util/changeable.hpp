#pragma once
#include <sigc++/sigc++.h>

namespace dune3d {
class Changeable {
public:
    typedef sigc::signal<void()> type_signal_changed;
    type_signal_changed signal_changed()
    {
        return m_signal_changed;
    }

protected:
    type_signal_changed m_signal_changed;
};
} // namespace dune3d

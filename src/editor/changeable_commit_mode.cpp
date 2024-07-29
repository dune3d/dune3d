#include "changeable_commit_mode.hpp"
#include "util/gtk_util.hpp"

namespace dune3d {
void ChangeableCommitMode::connect_spinbutton(Gtk::SpinButton &sp, std::function<bool()> fn)
{
    sp.signal_value_changed().connect([this, fn] {
        if (fn())
            m_signal_changed.emit(CommitMode::DELAYED);
    });

    spinbutton_connect_activate_immediate(sp, [this, fn] {
        if (fn())
            m_signal_changed.emit(CommitMode::IMMEDIATE);
    });
    {
        auto psp = &sp;
        auto controller = Gtk::EventControllerFocus::create();
        controller->signal_leave().connect([this, psp] {
            psp->update();
            m_signal_changed.emit(CommitMode::EXECUTE_DELAYED);
        });
        sp.add_controller(controller);
    }
}

} // namespace dune3d

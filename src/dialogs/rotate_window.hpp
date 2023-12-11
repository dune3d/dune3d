#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
#include "tool_window.hpp"
#include <glm/gtx/quaternion.hpp>

namespace dune3d {

class EditorInterface;
class SpinButtonAngle;

class ToolDataRotateWindow : public ToolDataWindow {
public:
    glm::dquat value;
};

class RotateWindow : public ToolWindow {
public:
    RotateWindow(Gtk::Window &parent, EditorInterface &intf, const std::string &label, const glm::dquat &initial);

    glm::dquat get_value() const;

private:
    SpinButtonAngle *m_sp_roll = nullptr;
    SpinButtonAngle *m_sp_pitch = nullptr;
    SpinButtonAngle *m_sp_yaw = nullptr;

    Gtk::ToggleButton *m_button_abs = nullptr;
    Gtk::ToggleButton *m_button_rel = nullptr;

    const glm::dquat m_initial;
    glm::dquat m_normal;

    void emit();
    void update_entries();
    void update_mode();
    bool m_updating = false;
};
} // namespace dune3d

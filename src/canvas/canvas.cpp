#include "canvas.hpp"
#include "selectable_ref.hpp"
#include "gl_util.hpp"
#include "import_step/import.hpp"
#include "bitmap_font_util.hpp"
#include "icon_texture_map.hpp"
#include "util/min_max_accumulator.hpp"
#include "util/template_util.hpp"
#include "logger/logger.hpp"
#include "iselection_filter.hpp"
#include <iostream>
#include <format>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/io.hpp>
#include <fstream>
#include "iselection_menu_creator.hpp"
#include "selectable_checkbutton.hpp"
#include "icon_texture_id.hpp"

#ifdef HAVE_SPNAV
#include <spnav.h>
#endif

namespace dune3d {

static const MSD::Params msd_params_slow{
        .mass = .0123,
        .damping = .2020,
        .springyness = 0.986,
};

[[maybe_unused]] static const MSD::Params msd_params_normal{
        .mass = 0.002,
        .damping = .2,
        .springyness = .15,
};

Canvas::Canvas()
    : m_background_renderer(*this), m_face_renderer(*this), m_line_renderer(*this), m_glyph_renderer(*this),
      m_glyph_3d_renderer(*this), m_icon_renderer(*this), m_picture_renderer(*this), m_box_selection(*this),
      m_selection_texture_renderer(*this)
{
    m_all_renderers.push_back(&m_face_renderer);
    m_all_renderers.push_back(&m_line_renderer);
    m_all_renderers.push_back(&m_glyph_renderer);
    m_all_renderers.push_back(&m_glyph_3d_renderer);
    m_all_renderers.push_back(&m_icon_renderer);
    m_all_renderers.push_back(&m_picture_renderer);
    set_can_focus(true);
    set_focusable(true);

    m_cam_quat = glm::quat_identity<float, glm::defaultp>();
    m_state.transform = glm::mat4(1);

#ifdef HAVE_SPNAV
    if (spnav_open() != -1) {
        if (auto fd = spnav_fd(); fd != -1) {
            have_spnav = true;
            auto chan = Glib::IOChannel::create_from_fd(fd);
            Logger::log_info("Connected to spacenavd", Logger::Domain::CANVAS);
            spnav_connection = Glib::signal_io().connect(
                    [this](Glib::IOCondition cond) {
                        if ((cond & Glib::IOCondition::IO_HUP) == Glib::IOCondition::IO_HUP) {
                            Logger::log_warning("disconnected from spacenavd", Logger::Domain::CANVAS);
                            return false;
                        }
                        handle_spnav();
                        return true;
                    },
                    chan, Glib::IOCondition::IO_IN | Glib::IOCondition::IO_HUP);
        }
    }
#endif

    m_animators.push_back(&m_quat_w_animator);
    m_animators.push_back(&m_quat_x_animator);
    m_animators.push_back(&m_quat_y_animator);
    m_animators.push_back(&m_quat_z_animator);
    m_animators.push_back(&m_zoom_animator);
    m_animators.push_back(&m_cx_animator);
    m_animators.push_back(&m_cy_animator);
    m_animators.push_back(&m_cz_animator);

    m_selection_menu = Gtk::make_managed<Gtk::Popover>();
    m_selection_menu->add_css_class("selection-menu");
    m_selection_menu->set_parent(*this);
    {
        auto label = Gtk::make_managed<Gtk::Label>("foo");
        m_selection_menu->set_child(*label);
    }
}

void Canvas::set_translation_rotation_animator_params(const MSD::Params &params)
{
    for (auto anim : {&m_quat_w_animator, &m_quat_x_animator, &m_quat_y_animator, &m_quat_z_animator, &m_cx_animator,
                      &m_cy_animator, &m_cz_animator}) {
        anim->set_params(params);
    }
}

#ifdef HAVE_SPNAV
void Canvas::handle_spnav()
{
    spnav_event e;
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    while (spnav_poll_event(&e)) {
        if (!top->is_active())
            continue;
        switch (e.type) {
        case SPNAV_EVENT_MOTION: {
            const auto thre = 10.0f;
            const auto values = {e.motion.x, e.motion.y, e.motion.z, e.motion.rx, e.motion.ry, e.motion.rz};
            if (std::any_of(values.begin(), values.end(), [thre](auto x) { return std::abs(x) > thre; })) {
                set_translation_rotation_animator_params(msd_params_normal);
                start_anim();
                m_animation_zoom_center = ZoomCenter::SCREEN;
                m_zoom_animator.target += e.motion.y * -0.001f;

                // reduce shifting speed when zoomed in
                const auto scale_shift = 0.16f * (1.0f - (1.0f / pow(2.2f, m_cam_distance)));
                const auto center_shift = get_center_shift(glm::vec2(e.motion.x, e.motion.z) * scale_shift);
                m_cx_animator.target += center_shift.x;
                m_cy_animator.target += center_shift.y;
                m_cz_animator.target += center_shift.z;

                auto scale_rot = -0.02f;
                const auto r = glm::quat(glm::radians(glm::vec3(e.motion.rx, e.motion.rz, e.motion.ry)) * scale_rot);
                const auto q = glm::normalize(m_cam_quat * r);
                m_quat_x_animator.target = q.x;
                m_quat_y_animator.target = q.y;
                m_quat_z_animator.target = q.z;
                m_quat_w_animator.target = q.w;
            }
        } break;
        }
    }
}
#endif

static glm::quat quat_from_between_vectors(glm::vec3 u, glm::vec3 v)
{
    // from https://github.com/rawify/Quaternion.js
    u = glm::normalize(u);
    v = glm::normalize(v);
    const auto dot = glm::dot(u, v);
    static const auto eps = 1e-5;
    if (dot >= 1 - eps) {
        return glm::quat_identity<float, glm::defaultp>();
    }
    if (1 + dot <= eps) {
        if (std::abs(u.x) > std::abs(u.z))
            return glm::normalize(glm::quat(0, -u.y, u.x, 0));
        else
            return glm::normalize(glm::quat(0, 0, -u.z, u.y));
    }

    const auto w = glm::cross(u, v);

    return glm::normalize(glm::quat(1 + dot, w.x, w.y, w.z));
}

void Canvas::setup_controllers()
{
    {
        auto controller = Gtk::EventControllerScroll::create();
        controller->set_flags(Gtk::EventControllerScroll::Flags::BOTH_AXES);
        // scroll_controller->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
        controller->signal_scroll().connect(
                [this, controller](double x, double y) {
                    if (controller->get_unit() == Gdk::ScrollUnit::SURFACE) {
                        x /= 15;
                        y /= 15;
                    }
                    const auto source = controller->get_current_event_device()->get_source();
                    const auto state = controller->get_current_event_state();
                    if (source == Gdk::InputSource::TRACKPOINT || source == Gdk::InputSource::TOUCHPAD) {
                        if ((state & Gdk::ModifierType::CONTROL_MASK) == Gdk::ModifierType::CONTROL_MASK)
                            scroll_zoom(x, y, *controller);
                        else if ((state & Gdk::ModifierType::SHIFT_MASK) == Gdk::ModifierType::SHIFT_MASK)
                            scroll_rotate(x, y, *controller);
                        else
                            scroll_move(x, y, *controller);
                        return GDK_EVENT_STOP;
                    }


                    scroll_zoom(x, y, *controller);
                    return GDK_EVENT_STOP;
                },
                false);
        add_controller(controller);
    }

    m_gesture_drag = Gtk::GestureDrag::create();
    m_gesture_drag->signal_begin().connect(sigc::mem_fun(*this, &Canvas::drag_gesture_begin_cb));
    m_gesture_drag->signal_update().connect(sigc::mem_fun(*this, &Canvas::drag_gesture_update_cb));
    m_gesture_drag->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    m_gesture_drag->set_touch_only(true);
    add_controller(m_gesture_drag);

    m_gesture_zoom = Gtk::GestureZoom::create();
    m_gesture_zoom->signal_begin().connect(sigc::mem_fun(*this, &Canvas::zoom_gesture_begin_cb));
    m_gesture_zoom->signal_update().connect(sigc::mem_fun(*this, &Canvas::zoom_gesture_update_cb));
    m_gesture_zoom->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    add_controller(m_gesture_zoom);

    m_gesture_rotate = Gtk::GestureRotate::create();
    m_gesture_rotate->signal_begin().connect(sigc::mem_fun(*this, &Canvas::rotate_gesture_begin_cb));
    m_gesture_rotate->signal_update().connect(sigc::mem_fun(*this, &Canvas::rotate_gesture_update_cb));
    m_gesture_rotate->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    add_controller(m_gesture_rotate);

    {
        auto controller = Gtk::GestureClick::create();
        controller->set_button(1);
        controller->signal_pressed().connect([this, controller](int n_press, double x, double y) {
            if (m_selection_mode == SelectionMode::NORMAL || m_selection_mode == SelectionMode::HOVER) {
                m_drag_selection_start = {x, y};
                m_dragging = true;
            }
            m_last_selection_mode = m_selection_mode;
            m_is_long_click = false;
            m_long_click_start = {x, y};

            if (m_hover_selection.has_value()) {
                m_long_click_connection = Glib::signal_timeout().connect(
                        [this] {
                            m_selection_peeling = true;
                            m_is_long_click = true;
                            queue_draw();
                            return false;
                        },
                        500);
            }
        });
        controller->signal_released().connect([this](int n_press, double x, double y) { handle_click_release(); });
        controller->signal_cancel().connect([this](auto seq) {
            m_long_click_connection.disconnect();
            m_dragging = false;
            m_inhibit_drag_selection = false;
            cancel_drag_selection();
        });
        add_controller(controller);
    }


    {
        auto controller = Gtk::GestureClick::create();
        controller->set_button(0);
        controller->signal_pressed().connect([this, controller](int n_press, double x, double y) {
            const auto shift = static_cast<bool>(controller->get_current_event_state() & Gdk::ModifierType::SHIFT_MASK);
            const auto ctrl =
                    static_cast<bool>(controller->get_current_event_state() & Gdk::ModifierType::CONTROL_MASK);
            const auto button = controller->get_current_button();
            if (button == 2 || button == 3) {
                m_pointer_pos_orig = {x, y};
                if (button == 3 && ctrl) {
                    m_pan_mode = PanMode::TILT;
                    m_cam_quat_orig = m_cam_quat;
                }
                else if (shift == (button == 2)) {
                    m_pan_mode = PanMode::ROTATE;
                    m_cam_quat_orig = m_cam_quat;
                }
                else {
                    m_pan_mode = PanMode::MOVE;
                    m_center_orig = m_center;
                }
            }
        });
        controller->signal_released().connect([this](int n_press, double x, double y) { end_pan(); });
        controller->signal_cancel().connect([this](Gdk::EventSequence *) { end_pan(); });
        add_controller(controller);
    }

    auto controller = Gtk::EventControllerMotion::create();
    controller->signal_motion().connect([this](double x, double y) {
        if (glm::length(m_long_click_start - glm::vec2(x, y)) > 16)
            m_long_click_connection.disconnect();

        grab_focus();
        const bool moved = m_last_x != x || m_last_y != y;
        if (!moved)
            return;
        m_last_x = x;
        m_last_y = y;
        m_cursor_pos.x = (x / m_width) * 2. - 1.;
        m_cursor_pos.y = (y / m_height) * -2. + 1.;
        if (m_dragging && !m_inhibit_drag_selection) {
            if (m_selection_mode == SelectionMode::DRAG) {
                update_drag_selection({x, y});
                return;
            }
            const auto delta_drag = glm::abs(glm::mat2(1, 0, 0, -1) * (glm::vec2(x, y) - m_drag_selection_start));
            if (delta_drag.x > 16 || delta_drag.y > 16) {
                set_selection_mode(SelectionMode::DRAG);
                m_box_selection.set_active(true);
                auto mask = VertexFlags::HOVER;
                clear_flags(mask);
                update_drag_selection({x, y});
                return;
            }
        }
        const auto delta = glm::mat2(1, 0, 0, -1) * (glm::vec2(x, y) - m_pointer_pos_orig);
        if (m_pan_mode == PanMode::ROTATE) {
            if (m_rotation_scheme == RotationScheme::ARCBALL) {
                const auto a = project_arcball(m_pointer_pos_orig);
                const auto b = project_arcball({x, y});
                auto q = quat_from_between_vectors(a, b);
                set_cam_quat(m_cam_quat_orig * glm::inverse(q * q));
            }
            else {
                glm::quat rz;
                if (m_rotation_scheme != RotationScheme::LEGACY)
                    rz = glm::angleAxis(glm::radians((delta.x / m_width) * -360),
                                        glm::rotate(m_cam_quat_orig, glm::vec3(0, 1, 0)));
                else
                    rz = glm::angleAxis(glm::radians((delta.x / m_width) * -360), glm::vec3(0, 0, 1));

                auto rx = glm::angleAxis(glm::radians((delta.y / m_height) * 90),
                                         glm::rotate(m_cam_quat_orig, glm::vec3(1, 0, 0)));
                set_cam_quat(rz * rx * m_cam_quat_orig);
            }
        }
        else if (m_pan_mode == PanMode::TILT) {
            auto ry = glm::angleAxis(glm::radians((delta.x / m_width) * 90),
                                     glm::rotate(m_cam_quat_orig, glm::vec3(0, 0, 1)));
            set_cam_quat(ry * m_cam_quat_orig);
        }
        else if (m_pan_mode == PanMode::MOVE) {
            m_center = m_center_orig + get_center_shift(delta);
            m_signal_view_changed.emit();
            queue_draw();
        }
        else {
            update_hover_selection();
            m_signal_cursor_moved.emit();
        }
    });
    add_controller(controller);
}

glm::vec3 Canvas::project_arcball(const glm::vec2 &v) const
{
    // from https://raw.org/code/trackball-rotation-using-quaternions/
    const float res = std::min(m_width, m_height) - 1;
    const auto p = glm::vec2((2 * v.x - m_width - 1), -(2 * v.y - m_height - 1)) / res;
    const auto d = glm::length2(p);
    float z;
    const float r = 1;
    if (2 * d <= r * r)
        z = sqrt(r * r - d);
    else
        z = r * r / 2 / sqrt(d);
    return {p.x, p.y, z};
}

bool Canvas::cancel_drag_selection()
{
    if (m_selection_mode != SelectionMode::DRAG)
        return false;

    m_box_selection.set_active(false);
    set_selection_mode(SelectionMode::NORMAL);
    m_signal_selection_changed.emit();
    return true;
}

void Canvas::handle_click_release()
{
    m_long_click_connection.disconnect();
    m_dragging = false;
    m_inhibit_drag_selection = false;
    if (m_last_selection_mode == SelectionMode::NONE || m_last_selection_mode == SelectionMode::HOVER_ONLY)
        return;
    if (cancel_drag_selection()) {
        // nop
    }
    else if (m_selection_mode == SelectionMode::HOVER) {
        if (m_hover_selection.has_value()) {
            m_selection_mode = SelectionMode::NORMAL;
            queue_draw();
            m_signal_selection_changed.emit();
            m_signal_selection_mode_changed.emit();
        }
    }
    else if (m_selection_mode == SelectionMode::NORMAL) {
        if (m_hover_selection.has_value()) {
            const auto state = get_display()->get_default_seat()->get_keyboard()->get_modifier_state();
            if ((state & Gdk::ModifierType::SHIFT_MASK) == Gdk::ModifierType::SHIFT_MASK) {
                m_selection_peeling = true;
                queue_draw();
                return;
            }
            auto sel = get_selection();
            if (sel.contains(m_hover_selection.value())) {
                sel.erase(m_hover_selection.value());
            }
            else {
                sel.insert(m_hover_selection.value());
            }
            set_selection(sel, true);
        }
        else {
            set_selection({}, true);
        }
    }
}

void Canvas::update_drag_selection(glm::vec2 pos)
{
    m_box_selection.set_box(m_drag_selection_start, pos);
    const auto a = glm::min(m_drag_selection_start, pos);
    const auto b = glm::max(m_drag_selection_start, pos);
    const auto x0 = static_cast<int>(a.x);
    const auto y0 = static_cast<int>(a.y);
    const auto x1 = static_cast<int>(b.x);
    const auto y1 = static_cast<int>(b.y);
    std::set<int> picks;
    std::set<int> picks_border;
    for (int x = x0; x <= x1; x++) {
        for (int y = y0; y <= y1; y++) {
            const bool is_border = (x == x0) || (x == x1) || (y == y0) || (y == y1);
            const auto pick = read_pick_buf(m_pick_buf, x, y);
            if (pick) {
                if (is_border)
                    picks_border.insert(pick);
                else
                    picks.insert(pick);
            }
        }
    }

    std::set<SelectableRef> sel;
    for (const auto pick : picks) {
        if (auto sr = get_selectable_ref_for_pick(pick))
            sel.insert(*sr);
    }
    for (const auto pick : picks_border) {
        if (auto sr = get_selectable_ref_for_pick(pick))
            sel.erase(*sr);
    }
    set_selection(sel, false);
}


void Canvas::end_pan()
{
    m_pan_mode = PanMode::NONE;
}

void Canvas::scroll_zoom(double dx, double dy, Gtk::EventController &ctrl)
{
    const auto zoom_center = m_zoom_to_cursor ? ZoomCenter::CURSOR : ZoomCenter::SCREEN;
    m_zoom_animator.set_params(msd_params_normal);
    animate_zoom_internal(dy, zoom_center);
}

void Canvas::scroll_move(double dx, double dy, Gtk::EventController &ctrl)
{
    auto delta = glm::vec2(dx * -83, dy * 83);
    m_center += get_center_shift(delta);
    m_signal_view_changed.emit();
    queue_draw();
}

void Canvas::scroll_rotate(double dx, double dy, Gtk::EventController &ctrl)
{
    auto delta = -glm::vec2(dx, dy);

    // auto rz = glm::angleAxis(glm::radians(delta.x * -9.f), glm::vec3(0, 0, 1));
    glm::quat rz;
    if (m_rotation_scheme != RotationScheme::LEGACY)
        rz = glm::angleAxis(glm::radians(delta.x * -9.f), glm::rotate(m_cam_quat, glm::vec3(0, 1, 0)));
    else
        rz = glm::angleAxis(glm::radians(delta.x * -9.f), glm::vec3(0, 0, 1));
    auto rx = glm::angleAxis(glm::radians(delta.y * -9.f), glm::rotate(m_cam_quat, glm::vec3(1, 0, 0)));
    set_cam_quat(rz * rx * m_cam_quat);
}


void Canvas::drag_gesture_begin_cb(Gdk::EventSequence *seq)
{
    if (m_pan_mode != PanMode::NONE) {
        m_gesture_drag->set_state(Gtk::EventSequenceState::DENIED);
    }
    else {
        m_gesture_drag_center_orig = m_center;
        const auto source = m_gesture_drag->get_current_event_device()->get_source();
        if (source == Gdk::InputSource::TOUCHSCREEN) {
            m_gesture_drag->set_state(Gtk::EventSequenceState::NONE);
        }
        else {
            m_gesture_drag->set_state(Gtk::EventSequenceState::CLAIMED);
        }
    }
}

void Canvas::drag_gesture_update_cb(Gdk::EventSequence *seq)
{
    double x, y;
    if (m_gesture_drag->get_offset(x, y)) {
        m_center = m_gesture_drag_center_orig + get_center_shift({x, -y});
        m_signal_view_changed.emit();
        queue_draw();
    }
}

void Canvas::zoom_gesture_begin_cb(Gdk::EventSequence *seq)
{
    if (m_pan_mode != PanMode::NONE) {
        m_gesture_zoom->set_state(Gtk::EventSequenceState::DENIED);
        return;
    }
    m_gesture_zoom_cam_dist_orig = m_cam_distance;
    m_gesture_zoom->set_state(Gtk::EventSequenceState::NONE);
}

void Canvas::zoom_gesture_update_cb(Gdk::EventSequence *seq)
{
    auto delta = m_gesture_zoom->get_scale_delta();
    set_cam_distance(m_gesture_zoom_cam_dist_orig / delta, ZoomCenter::CURSOR);
    queue_draw();
}

void Canvas::rotate_gesture_begin_cb(Gdk::EventSequence *seq)
{
    if (m_pan_mode != PanMode::NONE) {
        m_gesture_rotate->set_state(Gtk::EventSequenceState::DENIED);
        return;
    }
    m_gesture_rotate_cam_quat_orig = m_cam_quat;
    double cx, cy;
    m_gesture_rotate->get_bounding_box_center(cx, cy);
    m_gesture_rotate_pos_orig = glm::vec2(cx, cy);
    m_gesture_rotate->set_state(Gtk::EventSequenceState::NONE);
}

void Canvas::rotate_gesture_update_cb(Gdk::EventSequence *seq)
{
    auto delta = m_gesture_rotate->get_angle_delta();
    double cx, cy;
    m_gesture_rotate->get_bounding_box_center(cx, cy);
    float dy = cy - m_gesture_rotate_pos_orig.y;
    glm::quat rz;
    if (m_rotation_scheme != RotationScheme::LEGACY)
        rz = glm::angleAxis((float)delta, glm::rotate(m_gesture_rotate_cam_quat_orig, glm::vec3(0, 0, 1)));
    else
        rz = glm::angleAxis((float)delta, glm::vec3(0, 0, 1));
    auto rx = glm::angleAxis(glm::radians((dy / m_height) * -180),
                             glm::rotate(m_gesture_rotate_cam_quat_orig, glm::vec3(1, 0, 0)));
    set_cam_quat(rz * rx * m_gesture_rotate_cam_quat_orig);
}

void Canvas::set_cam_quat(const glm::quat &q)
{
    m_cam_quat = glm::normalize(q);
    queue_draw();
    m_signal_view_changed.emit();
}

void Canvas::set_cam_distance(float dist, ZoomCenter zoom_center)
{
    update_mats();
    glm::vec3 before = get_cursor_pos();
    m_cam_distance = dist;
    update_mats();
    glm::vec3 after = get_cursor_pos();
    if (zoom_center == ZoomCenter::CURSOR)
        set_center(get_center() - (after - before));
    queue_draw();
    m_signal_view_changed.emit();
}

void Canvas::animate_zoom(float factor, ZoomCenter zoom_center)
{
    m_zoom_animator.set_params(msd_params_slow);
    animate_zoom_internal(factor, zoom_center);
}

void Canvas::animate_zoom_internal(float factor, ZoomCenter zoom_center)
{
    const float zoom_base = 1.5;
    if (m_enable_animations) {
        if (factor == 0)
            return;
        start_anim();
        m_animation_zoom_center = zoom_center;
        m_zoom_animator.target += factor;
    }
    else {
        set_cam_distance(m_cam_distance * pow(zoom_base, factor), zoom_center);
    }
}

void Canvas::animate_pan(glm::vec2 shift)
{
    auto cs = get_center_shift(shift);
    if (m_enable_animations) {
        if (glm::length(shift) == 0)
            return;

        set_translation_rotation_animator_params(msd_params_slow);
        start_anim();
        m_cx_animator.target += cs.x;
        m_cy_animator.target += cs.y;
        m_cz_animator.target += cs.z;
    }
    else {
        set_center(get_center() + cs);
    }
}


void Canvas::set_center(glm::vec3 center)
{
    m_center = center;
    queue_draw();
    m_signal_view_changed.emit();
}

void Canvas::animate_to_cam_quat(const glm::quat &q)
{
    if (!m_enable_animations) {
        set_cam_quat(q);
        return;
    }
    set_translation_rotation_animator_params(msd_params_slow);
    start_anim();

    m_quat_w_animator.target = q.w;
    m_quat_x_animator.target = q.x;
    m_quat_y_animator.target = q.y;
    m_quat_z_animator.target = q.z;
}

void Canvas::animate_to_cam_quat_rel(const glm::quat &q)
{
    if (!m_enable_animations) {
        set_cam_quat(q * get_cam_quat());
        return;
    }
    set_translation_rotation_animator_params(msd_params_slow);
    start_anim();

    glm::quat qc{m_quat_w_animator.target, m_quat_x_animator.target, m_quat_y_animator.target,
                 m_quat_z_animator.target};
    qc = qc * q;
    m_quat_w_animator.target = qc.w;
    m_quat_x_animator.target = qc.x;
    m_quat_y_animator.target = qc.y;
    m_quat_z_animator.target = qc.z;
}

void Canvas::animate_to_center_abs(const glm::vec3 &center)
{
    if (!m_enable_animations) {
        set_center(center);
        return;
    }
    set_translation_rotation_animator_params(msd_params_slow);
    start_anim();

    m_cx_animator.target = center.x;
    m_cy_animator.target = center.y;
    m_cz_animator.target = center.z;
}

glm::quat Canvas::get_tilt_snapped_quat(const glm::quat &q) const
{
    const auto cq = get_cam_quat();

    float best_angle = 99;
    glm::quat best_q;
    for (int i = 0; i < 4; i++) {
        const auto tq = glm::angleAxis(-(float)(i * M_PI / 2), glm::vec3(0, 0, 1));
        const auto nq = q * tq;
        auto angle = glm::angle(nq * glm::inverse(cq));
        if (angle > M_PI)
            angle -= 2 * M_PI;
        if (std::abs(angle) < best_angle) {
            best_angle = std::abs(angle);
            best_q = nq;
        }
    }
    return best_q;
}

Canvas::VertexRef Canvas::get_vertex_ref_for_pick(unsigned int pick) const
{
    for (const auto &[key, it] : m_vertex_type_picks) {
        if ((pick >= it.offset) && ((pick - it.offset) < it.count))
            return {key.first, pick - it.offset, key.second};
    }
    throw std::runtime_error(std::format("pick {} not found", pick));
}

std::optional<SelectableRef> Canvas::get_selectable_ref_for_vertex_ref(const VertexRef &vref) const
{
    if (m_vertex_to_selectable_map.contains(vref)) {
        auto sr = m_vertex_to_selectable_map.at(vref);
        if (!m_selection_filter || m_selection_filter->can_select(sr))
            return sr;
        else
            return {};
    }
    else {
        return {};
    }
}

std::optional<SelectableRef> Canvas::get_selectable_ref_for_pick(unsigned int pick) const
{
    if (!pick)
        return {};
    return get_selectable_ref_for_vertex_ref(get_vertex_ref_for_pick(pick));
}

void Canvas::clear_flags(VertexFlags mask)
{
    for (auto &chunk : m_chunks) {
        chunk.clear_flags(mask);
    }
}

unsigned int Canvas::get_hover_pick(const std::vector<pick_buf_t> &pick_buf) const
{
    auto pick = read_pick_buf(pick_buf, m_last_x, m_last_y);
    if (!pick || any_of(get_vertex_ref_for_pick(pick).type, VertexType::FACE_GROUP, VertexType::PICTURE)) {
        int box_size = 10;
        float best_distance = glm::vec2(box_size, box_size).length();
        unsigned int best_pick = pick;
        for (int dx = -box_size; dx <= box_size; dx++) {
            for (int dy = -box_size; dy <= box_size; dy++) {
                int px = m_last_x + dx;
                int py = m_last_y + dy;
                if (px >= 0 && px < m_dev_width && py >= 0 && py < m_dev_height) {
                    if (auto p = read_pick_buf(pick_buf, px, py)) {
                        if (any_of(get_vertex_ref_for_pick(p).type, VertexType::FACE_GROUP, VertexType::PICTURE))
                            continue;
                        const auto d = glm::vec2(dx, dy).length();
                        if (d <= best_distance) {
                            best_distance = d;
                            best_pick = p;
                        }
                    }
                }
            }
        }
        pick = best_pick;
    }
    return pick;
}

unsigned int Canvas::get_hover_pick() const
{
    return get_hover_pick(m_pick_buf);
}


void Canvas::update_hover_selection()
{
    if (m_vertex_type_picks.size() == 0)
        return;
    if (m_selection_mode != SelectionMode::NONE) {
        auto last_hover_selection = m_hover_selection;
        m_hover_selection.reset();
        auto pick = get_hover_pick();

        if (auto sr = get_selectable_ref_for_pick(pick))
            m_hover_selection = *sr;


        if (m_hover_selection != last_hover_selection) {
            auto mask = VertexFlags::HOVER;
            if (m_selection_mode == SelectionMode::HOVER || m_selection_mode == SelectionMode::HOVER_ONLY) {
                mask |= VertexFlags::SELECTED;
            }
            clear_flags(mask);
            if (m_hover_selection.has_value()) {
                for (const auto &vref : m_selectable_to_vertex_map.at(m_hover_selection.value())) {
                    auto &flags = get_vertex_flags(vref);
                    flags |= mask;
                }
            }
            m_push_flags =
                    static_cast<PushFlags>(m_push_flags | PF_LINES | PF_GLYPHS | PF_GLYPHS_3D | PF_ICONS | PF_PICTURES);
            queue_draw();
            m_signal_hover_selection_changed.emit();
        }
    }
}

Canvas::pick_buf_t Canvas::read_pick_buf(const std::vector<pick_buf_t> &pick_buf, int x, int y) const
{
    int xi = x * m_scale_factor;
    int yi = y * m_scale_factor;
    if (xi >= m_dev_width || yi >= m_dev_height || x < 0 || y < 0)
        return 0;
    const int idx = ((m_dev_height)-yi - 1) * m_dev_width + xi;
    return pick_buf.at(idx);
}

glm::dvec3 Canvas::get_cursor_pos_for_plane(glm::dvec3 origin, glm::dvec3 normal) const
{
    auto r = m_projmat_viewmat_inv * glm::dvec4(m_cursor_pos, -1, 1);
    r /= r.w;
    auto r2 = m_projmat_viewmat_inv * glm::dvec4(m_cursor_pos, 1, 1);
    r2 /= r2.w;
    const auto mouse_normal = glm::dvec3(glm::normalize(r2 - r));

    auto d = glm::dot(origin - glm::dvec3(r), normal) / glm::dot(normal, mouse_normal);
    return glm::dvec3(r) + d * mouse_normal;
}

glm::dvec3 Canvas::get_cursor_pos() const
{
    return get_cursor_pos_for_plane(m_center, m_cam_normal);
}

glm::vec3 Canvas::get_cam_normal() const
{
    return m_cam_normal;
}

glm::dvec2 Canvas::get_cursor_pos_win() const
{
    return {m_last_x, m_last_y};
}

float Canvas::get_magic_number() const
{
    return tan(0.5 * glm::radians(m_cam_fov)) / m_dev_height * m_cam_distance;
}

glm::vec3 Canvas::get_center_shift(const glm::vec2 &shift) const
{
    const float m = 2 * get_magic_number();
    auto s = -shift * m * (float)m_scale_factor;
    return glm::rotate(m_cam_quat, glm::vec3(s, 0));
}


void Canvas::on_realize()
{
    std::cout << "realize" << std::endl;
#if GTK_CHECK_VERSION(4, 12, 0)
    gtk_gl_area_set_allowed_apis(gobj(), GDK_GL_API_GL);
#endif
    Gtk::GLArea::on_realize();
    make_current();
    m_background_renderer.realize();
    m_face_renderer.realize();
    m_line_renderer.realize();
    m_glyph_renderer.realize();
    m_glyph_3d_renderer.realize();
    m_icon_renderer.realize();
    m_picture_renderer.realize();
    m_box_selection.realize();
    m_selection_texture_renderer.realize();
    GL_CHECK_ERROR


    glEnable(GL_DEPTH_TEST);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);

    GLint fb;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fb); // save fb

    glGenRenderbuffers(1, &m_renderbuffer);
    glGenRenderbuffers(1, &m_depthrenderbuffer);
    glGenRenderbuffers(1, &m_pickrenderbuffer);
    glGenRenderbuffers(1, &m_pickrenderbuffer_downsampled);
    glGenRenderbuffers(1, &m_last_frame_renderbuffer);
    glGenTextures(1, &m_selection_texture);

    resize_buffers();

    GL_CHECK_ERROR

    glGenFramebuffers(1, &m_fbo_downsampled);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_downsampled);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_pickrenderbuffer_downsampled);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        // Gtk::MessageDialog md("Error setting up framebuffer, will now exit", false /* use_markup */,
        // Gtk::MESSAGE_ERROR,
        //                      Gtk::BUTTONS_OK);
        // md.run();
        abort();
    }

    GL_CHECK_ERROR

    glGenFramebuffers(1, &m_fbo_last_frame);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_last_frame);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_last_frame_renderbuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        // Gtk::MessageDialog md("Error setting up framebuffer, will now exit", false /* use_markup */,
        // Gtk::MESSAGE_ERROR,
        //                      Gtk::BUTTONS_OK);
        // md.run();
        abort();
    }

    GL_CHECK_ERROR

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_renderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, m_pickrenderbuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D_MULTISAMPLE, m_selection_texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthrenderbuffer);

    GL_CHECK_ERROR

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        // Gtk::MessageDialog md("Error setting up framebuffer, will now exit", false /* use_markup */,
        // Gtk::MESSAGE_ERROR,
        //                      Gtk::BUTTONS_OK);
        // md.run();
        abort();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fb);

    GL_CHECK_ERROR
}

GLint Canvas::get_samples() const
{
#ifndef __APPLE__
    return m_appearance.msaa;
#else
    // multisampling does not seem to work on macOS
    // setting the sampling factor to 0 will fall back to the non-multisampled behavior
    return 0;
#endif
}

void Canvas::resize_buffers()
{
    GLint rb;
    const auto samples = get_samples();

    glGetIntegerv(GL_RENDERBUFFER_BINDING, &rb); // save rb
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, m_dev_width, m_dev_height);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthrenderbuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, m_dev_width, m_dev_height);
    glBindRenderbuffer(GL_RENDERBUFFER, m_pickrenderbuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_R32UI, m_dev_width, m_dev_height);

    glBindRenderbuffer(GL_RENDERBUFFER, m_pickrenderbuffer_downsampled);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_R32UI, m_dev_width, m_dev_height);

    glBindRenderbuffer(GL_RENDERBUFFER, m_last_frame_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, m_dev_width, m_dev_height);

    glBindRenderbuffer(GL_RENDERBUFFER, rb);


    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_selection_texture);
    GL_CHECK_ERROR
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, m_dev_width, m_dev_height, GL_TRUE);
    GL_CHECK_ERROR
}


void Canvas::set_chunk(unsigned int chunk)
{
    if (m_chunks.size() < chunk + 1)
        m_chunks.resize(chunk + 1);
    m_current_chunk_id = chunk;
    m_current_chunk = &m_chunks.at(chunk);
}

std::vector<unsigned int> Canvas::get_chunk_ids() const
{
    std::vector<unsigned int> chunk_ids;
    chunk_ids.reserve(m_chunks.size());
    for (unsigned int i = 0; i < m_chunks.size(); i++) {
        chunk_ids.push_back(m_chunks.size() - 1 - i);
    }
    return chunk_ids;
}

ICanvas::VertexRef Canvas::add_face_group(const face::Faces &faces, glm::vec3 origin, glm::quat normal,
                                          FaceColor face_color)
{
    auto offset = m_current_chunk->m_face_index_buffer.size();
    add_faces(faces);
    auto length = m_current_chunk->m_face_index_buffer.size() - offset;
    m_current_chunk->m_face_groups.push_back(CanvasChunk::FaceGroup{
            .offset = offset,
            .length = length,
            .origin = origin,
            .normal = normal,
            .color = face_color,
    });

    return {VertexType::FACE_GROUP, m_current_chunk->m_face_groups.size() - 1, m_current_chunk_id};
}

void Canvas::add_faces(const face::Faces &faces)
{
    size_t vertex_offset = m_current_chunk->m_face_vertex_buffer.size();
    for (const auto &face : faces) {
        for (size_t i = 0; i < face.vertices.size(); i++) {
            const auto &v = face.vertices.at(i);
            const auto &n = face.normals.at(i);
            auto vt = transform_point({v.x, v.y, v.z});
            auto nt = transform_point_rel({n.x, n.y, n.z});
            m_current_chunk->m_face_vertex_buffer.emplace_back(vt.x, vt.y, vt.z, nt.x, nt.y, nt.z, face.color.r * 255,
                                                               face.color.g * 255, face.color.b * 255);
        }

        for (const auto &tri : face.triangle_indices) {
            size_t a, b, c;
            std::tie(a, b, c) = tri;
            m_current_chunk->m_face_index_buffer.push_back(a + vertex_offset);
            m_current_chunk->m_face_index_buffer.push_back(b + vertex_offset);
            m_current_chunk->m_face_index_buffer.push_back(c + vertex_offset);
        }
        vertex_offset += face.vertices.size();
    }
}

void Canvas::update_mats()
{
    float r = m_cam_distance;
    auto cam_offset = glm::rotate(m_cam_quat, glm::vec3(0, 0, r));
    auto cam_pos = cam_offset + m_center;


    m_viewmat = glm::lookAt(cam_pos, m_center, glm::rotate(m_cam_quat, glm::vec3(0, 1, 0)));

    float cam_dist_min = 1e6;
    float cam_dist_max = -1e6;

    std::array<glm::vec3, 8> bbs = {glm::vec3(m_bbox.first.x, m_bbox.first.y, m_bbox.first.z),
                                    glm::vec3(m_bbox.first.x, m_bbox.second.y, m_bbox.first.z),
                                    glm::vec3(m_bbox.second.x, m_bbox.first.y, m_bbox.first.z),
                                    glm::vec3(m_bbox.second.x, m_bbox.second.y, m_bbox.first.z),
                                    glm::vec3(m_bbox.first.x, m_bbox.first.y, m_bbox.second.z),
                                    glm::vec3(m_bbox.first.x, m_bbox.second.y, m_bbox.second.z),
                                    glm::vec3(m_bbox.second.x, m_bbox.first.y, m_bbox.second.z),
                                    glm::vec3(m_bbox.second.x, m_bbox.second.y, m_bbox.second.z)};

    for (const auto &bb : bbs) {
        float dist = glm::length(bb - cam_pos);
        cam_dist_max = std::max(dist, cam_dist_max);
        cam_dist_min = std::min(dist, cam_dist_min);
    }

    const float m = get_magic_number();
    float d = cam_dist_max * 2;

    if (m_projection == Projection::PERSP) {
        m_projmat = glm::perspective(glm::radians(m_cam_fov), (float)m_dev_width / m_dev_height, cam_dist_min / 2,
                                     cam_dist_max * 2);
    }
    else {
        m_projmat = glm::ortho(-m_dev_width * m, m_dev_width * m, -m_dev_height * m, m_dev_height * m, -d, d);
    }
    m_cam_normal = glm::normalize(cam_offset);

    m_projmat_viewmat_inv = glm::inverse(m_projmat * m_viewmat);
}

void Canvas::render_all(std::vector<pick_buf_t> &pick_buf)
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glClearColor(0, 0, 0, 0);
    glClearDepth(10);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GL_CHECK_ERROR
    {
        const std::array<GLenum, 3> bufs = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
        glDrawBuffers(bufs.size(), bufs.data());
    }


    glDisable(GL_DEPTH_TEST);
    m_background_renderer.render();
    glEnable(GL_DEPTH_TEST);


    if (m_push_flags != PF_NONE) {
        m_pick_base = 1;
        m_face_renderer.push();
        m_line_renderer.push();
        m_glyph_renderer.push();
        m_glyph_3d_renderer.push();
        m_icon_renderer.push();
        m_picture_renderer.push();
    }
    m_push_flags = PF_NONE;

    update_mats();

    m_face_renderer.render();
    GL_CHECK_ERROR
    // glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnablei(GL_BLEND, 0);
    glEnablei(GL_BLEND, 2);
    m_icon_renderer.render();
    m_line_renderer.render();

    m_glyph_renderer.render();
    m_glyph_3d_renderer.render();

    m_picture_renderer.render();

    glDisable(GL_DEPTH_TEST);
    m_box_selection.render();
    if (m_show_error_overlay)
        m_background_renderer.render_error();

    if (!m_selection_peeling && m_appearance.selection_glow) {
        glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
        glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glColorMaski(2, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        m_selection_texture_renderer.render();
        glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColorMaski(2, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
    }
    glEnable(GL_DEPTH_TEST);

    glDisablei(GL_BLEND, 0);
    glDisablei(GL_BLEND, 2);
    // glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    GL_CHECK_ERROR

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_downsampled);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glBlitFramebuffer(0, 0, m_dev_width, m_dev_height, 0, 0, m_dev_width, m_dev_height, GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_downsampled);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    pick_buf.resize(m_dev_width * m_dev_height);
    GL_CHECK_ERROR

    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(0, 0, m_dev_width, m_dev_height, GL_RED_INTEGER, GL_UNSIGNED_INT, pick_buf.data());
}

void Canvas::peel_selection()
{
    std::vector<pick_buf_t> pick_buf;
    std::vector<unsigned int> peeled_picks;
    for (auto pick = get_hover_pick(m_pick_buf); pick; pick = get_hover_pick(pick_buf)) {
        peeled_picks.push_back(pick);
        if (peeled_picks.size() > s_peel_max) {
            break;
        }
        for (auto renderer : m_all_renderers) {
            renderer->set_peeled_picks(peeled_picks);
        }
        render_all(pick_buf);
    }

    ISelectionMenuCreator::SelectableRefAndVertexTypeList srv_list;

    {
        std::set<SelectableRef> seen_selctables;
        for (const auto pick : peeled_picks) {
            const auto vref = get_vertex_ref_for_pick(pick);
            const auto sr = get_selectable_ref_for_vertex_ref(vref);
            if (sr.has_value()) {
                if (!seen_selctables.insert(sr.value()).second)
                    continue;
            }
            // only add face groups if they're first
            bool add = true;
            if (vref.type == VertexType::FACE_GROUP)
                add = srv_list.size() == 0;
            if (add)
                srv_list.emplace_back(vref.type, sr);

            if (vref.type == VertexType::FACE_GROUP)
                break;
        }
    }
    bool need_menu = false;
    std::optional<SelectableRef> the_selectable;
    if (srv_list.size() == 0) {
        need_menu = false;
    }
    else if (!srv_list.front().selectable.has_value()) { // first item is not selectable, should not happen
        need_menu = false;
    }
    else if (srv_list.size() == 1) {
        need_menu = false;
        the_selectable = srv_list.front().selectable;
    }
    else if (srv_list.size() == 2) {
        const auto &a = srv_list.front().selectable;
        const auto &b = srv_list.back().selectable;
        if (a.has_value() && b.has_value() && a->is_entity() && b->is_entity() && a->item == b->item && a->point != 0
            && b->point == 0) {
            the_selectable = *a;
            need_menu = false;
        }
        else {
            need_menu = true;
        }
    }
    else {
        need_menu = true;
    }

    if (the_selectable.has_value()) {
        auto sel = get_selection();
        if (sel.contains(the_selectable.value()))
            sel.erase(the_selectable.value());
        else
            sel.insert(the_selectable.value());
        Glib::signal_idle().connect_once([this, sel] { set_selection(sel, true); });
    }


    if (need_menu) {
        Gdk::Rectangle rect;
        rect.set_x(m_last_x);
        rect.set_y(m_last_y);
        const bool is_hover_only = m_selection_mode == SelectionMode::HOVER_ONLY;
        if (m_selection_menu_creator) {
            auto buttons = m_selection_menu_creator->create(*m_selection_menu, srv_list);
            auto sel = get_selection();
            for (auto button : buttons) {
                if (is_hover_only) {
                    button->signal_toggled().connect([this, button] {
                        m_selection_menu->popdown();
                        m_hover_selection = button->m_selectable;
                        m_signal_select_from_menu.emit(button->m_selectable);
                    });
                }
                else {
                    if (m_selection_mode != SelectionMode::HOVER)
                        button->set_active(sel.contains(button->m_selectable));
                    button->signal_toggled().connect([this, button] {
                        std::set<SelectableRef> sel2;

                        if (m_selection_mode != SelectionMode::HOVER)
                            sel2 = get_selection();

                        if (button->get_active())
                            sel2.insert(button->m_selectable);
                        else
                            sel2.erase(button->m_selectable);

                        set_selection(sel2, true);
                        set_selection_mode(SelectionMode::NORMAL);
                        const auto state =
                                button->get_display()->get_default_seat()->get_keyboard()->get_modifier_state();
                        if ((state & Gdk::ModifierType::SHIFT_MASK) != Gdk::ModifierType::SHIFT_MASK)
                            m_selection_menu->popdown();
                    });
                }
                auto controller = Gtk::EventControllerMotion::create();
                controller->signal_leave().connect([this] { set_highlight({}); });
                controller->signal_enter().connect(
                        [this, button](double, double) { set_highlight({button->m_selectable}); });
                button->add_controller(controller);
            }
        }

        m_selection_menu->set_pointing_to(rect);
        Glib::signal_idle().connect_once([this] { m_selection_menu->popup(); });
    }
}

bool Canvas::on_render(const Glib::RefPtr<Gdk::GLContext> &context)
{
    const bool first_render = m_vertex_type_picks.size() == 0;

    Gtk::GLArea::on_render(context);

    if (m_needs_resize) {
        resize_buffers();
        m_needs_resize = false;
    }

    // fixes glitches on AMD??
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_selection_texture);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, get_samples(), GL_RGBA8, m_dev_width, m_dev_height, GL_TRUE);

    GLint fb;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fb); // save fb


#ifdef __APPLE__
    glDisable(GL_MULTISAMPLE);
#endif


    if (m_selection_peeling && !first_render) {
        peel_selection();
    }
    else {
        for (auto renderer : m_all_renderers) {
            renderer->set_peeled_picks({});
        }
        render_all(m_pick_buf);
    }


    GL_CHECK_ERROR
    if (m_pick_state == PickState::QUEUED) {
        m_pick_state = PickState::CURRENT;
        std::ofstream ofs(m_pick_path.string());
        for (int y = 0; y < m_dev_height; y++) {
            for (int x = 0; x < m_dev_width; x++) {
                ofs << m_pick_buf.at(x + y * m_dev_width) << " ";
            }
            ofs << std::endl;
        }
    }


    if (!m_selection_peeling) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_last_frame);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBlitFramebuffer(0, 0, m_dev_width, m_dev_height, 0, 0, m_dev_width, m_dev_height, GL_COLOR_BUFFER_BIT,
                          GL_NEAREST);
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_last_frame);
    glDrawBuffer(fb ? GL_COLOR_ATTACHMENT0 : GL_FRONT);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, m_dev_width, m_dev_height, 0, 0, m_dev_width, m_dev_height, GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    GL_CHECK_ERROR
    glFlush();

    m_selection_peeling = false;

    if (first_render) {
        update_hover_selection();
    }

    return true;
}

void Canvas::on_resize(int width, int height)
{
    const auto scale_factor = get_scale_factor();
    if (width == m_dev_width && height == m_dev_height && scale_factor == m_scale_factor)
        return;
    std::cout << "resize " << width << "x" << height << std::endl;
    m_dev_width = width;
    m_dev_height = height;
    m_height = get_height();
    m_width = get_width();
    m_screenmat = glm::scale(glm::translate(glm::mat3(1), glm::vec2(-1, 1)),
                             glm::vec2(2.0 / m_dev_width, -2.0 / m_dev_height));

    resize_buffers();

    // get_scale_factor is surprisingly expensive, so do it only once
    m_scale_factor = scale_factor;

    Gtk::GLArea::on_resize(width, height);
}

void Canvas::request_push()
{
    m_push_flags = PF_ALL;
    queue_draw();
}

void Canvas::queue_pick(const std::filesystem::path &pick_path)
{
    m_pick_state = PickState::QUEUED;
    m_pick_path = pick_path;
    queue_draw();
}

void Canvas::clear()
{
    for (auto &chunk : m_chunks) {
        chunk.clear();
    }
    m_selectable_to_vertex_map.clear();
    m_vertex_to_selectable_map.clear();
    m_vertex_type_picks.clear();
    m_push_flags = PF_ALL;
    queue_draw();
}

void Canvas::clear_chunks(unsigned int first_chunk)
{
    unsigned int chunk_id = 0;
    for (auto &chunk : m_chunks) {
        if (chunk_id >= first_chunk)
            chunk.clear();
        chunk_id++;
    }

    for (auto it = m_vertex_to_selectable_map.cbegin(); it != m_vertex_to_selectable_map.cend() /* not hoisted */;
         /* no increment */) {
        if (it->first.chunk >= first_chunk) {
            m_selectable_to_vertex_map.erase(it->second);
            it = m_vertex_to_selectable_map.erase(it);
        }
        else {
            ++it;
        }
    }

    m_vertex_type_picks.clear();
    m_push_flags = PF_ALL;
    queue_draw();
}

ICanvas::VertexRef Canvas::draw_point(glm::vec3 p)
{
    return draw_point(p, IconTexture::IconTextureID::POINT_BOX);
}

ICanvas::VertexRef Canvas::draw_line(glm::vec3 a, glm::vec3 b)
{
    auto &lines = m_state.selection_invisible ? m_current_chunk->m_lines_selection_invisible : m_current_chunk->m_lines;
    auto &li = lines.emplace_back(transform_point(a), transform_point(b));
    apply_flags(li.flags);
    apply_line_flags(li.flags);

    if (m_state.selection_invisible)
        return {VertexType::SELECTION_INVISIBLE, 0};
    return {VertexType::LINE, m_current_chunk->m_lines.size() - 1, m_current_chunk_id};
}

ICanvas::VertexRef Canvas::draw_screen_line(glm::vec3 a, glm::vec3 b)
{
    auto &lines = m_state.selection_invisible ? m_current_chunk->m_lines_selection_invisible : m_current_chunk->m_lines;

    auto &li = lines.emplace_back(transform_point(a), transform_point_rel(b));
    li.flags |= VertexFlags::SCREEN;
    apply_flags(li.flags);
    apply_line_flags(li.flags);
    if (m_state.selection_invisible)
        return {VertexType::SELECTION_INVISIBLE, 0};
    return {VertexType::LINE, m_current_chunk->m_lines.size() - 1, m_current_chunk_id};
}

void Canvas::apply_flags(VertexFlags &flags)
{
    if (m_state.vertex_inactive)
        flags |= VertexFlags::INACTIVE;
    if (m_state.vertex_constraint)
        flags |= VertexFlags::CONSTRAINT;
    if (m_state.vertex_construction)
        flags |= VertexFlags::CONSTRUCTION;
}

void Canvas::apply_line_flags(VertexFlags &flags)
{
    if (m_state.line_style == LineStyle::THIN)
        flags |= VertexFlags::LINE_THIN;
}

static const float char_space = 1;

static uint32_t pack_bits(const bitmap_font::GlyphInfo &info)
{
    return (info.get_h() & 0x3f) | ((info.get_w() & 0x3f) << 6) | ((info.get_y() & 0x3ff) << 12)
           | ((info.get_x() & 0x3ff) << 22);
}

std::vector<ICanvas::VertexRef> Canvas::draw_bitmap_text(glm::vec3 p, float size, const std::string &rtext)
{
    p = transform_point(p);
    std::vector<ICanvas::VertexRef> vrefs;
    Glib::ustring text(rtext);
    float sc = size * .75;

    glm::vec2 point = {0, 0};

    glm::vec2 v = {1, 0};
    for (auto codepoint : text) {
        if (codepoint != ' ') {
            auto info = bitmap_font::get_glyph_info(codepoint);
            if (!info.is_valid()) {
                info = bitmap_font::get_glyph_info('?');
            }

            const uint32_t bits = pack_bits(info);

            glm::vec2 shift(info.minx, -info.miny);

            auto ps = point + shift * sc;

            auto &gl = m_current_chunk->m_glyphs.emplace_back(p.x, p.y, p.z, ps.x, ps.y, sc, bits);
            apply_flags(gl.flags);


            vrefs.push_back({VertexType::GLYPH, m_current_chunk->m_glyphs.size() - 1, m_current_chunk_id});

            point += v * (info.advance * char_space * sc);
        }
        else {
            point += v * (7 * char_space * sc);
        }
    }


    return vrefs;
}

std::vector<ICanvas::VertexRef> Canvas::draw_bitmap_text_3d(glm::vec3 p, const glm::quat &norm_in, float size,
                                                            const std::string &rtext)
{
    p = transform_point(p);
    auto norm = glm::quat_cast(m_state.transform) * norm_in;
    std::vector<ICanvas::VertexRef> vrefs;
    Glib::ustring text(rtext);

    float sc = size * .0546;

    glm::vec3 point = {0, 0, 0};

    glm::vec3 right = glm::rotate(norm, glm::vec3(1, 0, 0));
    glm::vec3 up = glm::rotate(norm, glm::vec3(0, 1, 0));
    glm::vec3 v = {1, 0, 0};
    for (auto codepoint : text) {
        if (codepoint != ' ') {
            auto info = bitmap_font::get_glyph_info(codepoint);
            if (!info.is_valid()) {
                info = bitmap_font::get_glyph_info('?');
            }

            const uint32_t bits = pack_bits(info);

            const glm::vec3 shift(info.minx, info.miny, 0);

            const auto ps = point + shift * sc;
            const auto pt = p + glm::rotate(norm, ps);
            const auto r = right * (float)info.get_w() * sc;
            const auto u = up * (float)info.get_h() * sc;
            auto &gl = m_current_chunk->m_glyphs_3d.emplace_back(pt.x, pt.y, pt.z, r.x, r.y, r.z, u.x, u.y, u.z, bits);
            apply_flags(gl.flags);

            vrefs.push_back({VertexType::GLYPH_3D, m_current_chunk->m_glyphs_3d.size() - 1, m_current_chunk_id});

            point += v * (info.advance * char_space * sc);
        }
        else {
            point += v * (7 * char_space * sc);
        }
    }


    return vrefs;
}

ICanvas::VertexRef Canvas::draw_icon(IconTexture::IconTextureID id, glm::vec3 origin, glm::vec2 shift, glm::vec3 v)
{
    origin = transform_point(origin);
    auto &icons = m_state.selection_invisible ? m_current_chunk->m_icons_selection_invisible : m_current_chunk->m_icons;
    auto icon_pos = IconTexture::icon_texture_map.at(id);
    auto &icon =
            icons.emplace_back(origin.x, origin.y, origin.z, shift.x, shift.y, v.x, v.y, v.z, icon_pos.x, icon_pos.y);
    apply_flags(icon.flags);
    if (m_state.selection_invisible)
        return {VertexType::SELECTION_INVISIBLE, 0};
    return {VertexType::ICON, m_current_chunk->m_icons.size() - 1, m_current_chunk_id};
}


ICanvas::VertexRef Canvas::draw_point(glm::vec3 p, IconTexture::IconTextureID id)
{
    if (m_state.no_points)
        return {VertexType::SELECTION_INVISIBLE, 0};

    return draw_icon(id, p, {0, 0}, {NAN, NAN, NAN});
}

ICanvas::VertexRef Canvas::draw_picture(const std::array<glm::vec3, 4> &corners,
                                        std::shared_ptr<const PictureData> data)
{
    m_current_chunk->m_pictures.push_back(CanvasChunk::Picture{
            .corners = corners,
            .selection_invisible = m_state.selection_invisible,
            .data = data,
    });

    return {VertexType::PICTURE, m_current_chunk->m_pictures.size() - 1, m_current_chunk_id};
}


void Canvas::add_selectable(const VertexRef &vref, const SelectableRef &sref)
{
    if (vref.type == VertexType::SELECTION_INVISIBLE)
        return;
    SelectableRef sr = sref;
    if (m_override_selectable.has_value())
        sr = m_override_selectable.value();
    m_vertex_to_selectable_map.emplace(vref, sr);
    m_selectable_to_vertex_map[sr].push_back(vref);
}

Canvas::VertexFlags &Canvas::get_vertex_flags(const VertexRef &vref)
{
    return m_chunks.at(vref.chunk).get_vertex_flags(vref);
}

void Canvas::set_selection(const std::set<SelectableRef> &sel, bool emit)
{
    set_flag_for_selectables(sel, VertexFlags::SELECTED);
    if (emit)
        m_signal_selection_changed.emit();
}

void Canvas::set_highlight(const std::set<SelectableRef> &sel)
{
    set_flag_for_selectables(sel, VertexFlags::HIGHLIGHT);
}

void Canvas::set_flag_for_selectables(const std::set<SelectableRef> &sel, VertexFlags flag)
{
    clear_flags(flag);
    for (auto &sr : sel) {
        if (!m_selectable_to_vertex_map.contains(sr))
            continue;
        auto &vrefs = m_selectable_to_vertex_map.at(sr);
        for (const auto &vref : vrefs) {
            auto &flags = get_vertex_flags(vref);
            flags |= flag;
        }
        // auto &flags = get_vertex_flags()
    }
    m_push_flags = static_cast<PushFlags>(m_push_flags | PF_LINES | PF_GLYPHS | PF_GLYPHS_3D | PF_ICONS | PF_PICTURES);
    queue_draw();
}

void Canvas::set_hover_selection(const std::optional<SelectableRef> &sr)
{
    if (!sr.has_value())
        return;
    if (!m_selectable_to_vertex_map.contains(*sr))
        return;
    auto &vrefs = m_selectable_to_vertex_map.at(*sr);
    for (const auto &vref : vrefs) {
        auto &flags = get_vertex_flags(vref);
        flags |= VertexFlags::HOVER;
    }
}

std::set<SelectableRef> Canvas::get_selection() const
{
    std::set<SelectableRef> r;
    unsigned int chunk_id = 0;
    for (auto &chunk : m_chunks) {
        for (size_t i = 0; i < chunk.m_lines.size(); i++) {
            if ((chunk.m_lines.at(i).flags & VertexFlags::SELECTED) != VertexFlags::DEFAULT) {
                const VertexRef vref{.type = VertexType::LINE, .index = i, .chunk = chunk_id};
                if (m_vertex_to_selectable_map.count(vref))
                    r.insert(m_vertex_to_selectable_map.at(vref));
            }
        }
        for (size_t i = 0; i < chunk.m_glyphs.size(); i++) {
            if ((chunk.m_glyphs.at(i).flags & VertexFlags::SELECTED) != VertexFlags::DEFAULT) {
                const VertexRef vref{.type = VertexType::GLYPH, .index = i, .chunk = chunk_id};
                if (m_vertex_to_selectable_map.count(vref))
                    r.insert(m_vertex_to_selectable_map.at(vref));
            }
        }
        for (size_t i = 0; i < chunk.m_glyphs_3d.size(); i++) {
            if ((chunk.m_glyphs_3d.at(i).flags & VertexFlags::SELECTED) != VertexFlags::DEFAULT) {
                const VertexRef vref{.type = VertexType::GLYPH_3D, .index = i, .chunk = chunk_id};
                if (m_vertex_to_selectable_map.count(vref))
                    r.insert(m_vertex_to_selectable_map.at(vref));
            }
        }
        for (size_t i = 0; i < chunk.m_face_groups.size(); i++) {
            if ((chunk.m_face_groups.at(i).flags & VertexFlags::SELECTED) != VertexFlags::DEFAULT) {
                const VertexRef vref{.type = VertexType::FACE_GROUP, .index = i, .chunk = chunk_id};
                if (m_vertex_to_selectable_map.count(vref))
                    r.insert(m_vertex_to_selectable_map.at(vref));
            }
        }
        for (size_t i = 0; i < chunk.m_icons.size(); i++) {
            if ((chunk.m_icons.at(i).flags & VertexFlags::SELECTED) != VertexFlags::DEFAULT) {
                const VertexRef vref{.type = VertexType::ICON, .index = i, .chunk = chunk_id};
                if (m_vertex_to_selectable_map.count(vref))
                    r.insert(m_vertex_to_selectable_map.at(vref));
            }
        }
        for (size_t i = 0; i < chunk.m_pictures.size(); i++) {
            if ((chunk.m_pictures.at(i).flags & VertexFlags::SELECTED) != VertexFlags::DEFAULT) {
                const VertexRef vref{.type = VertexType::PICTURE, .index = i, .chunk = chunk_id};
                if (m_vertex_to_selectable_map.count(vref))
                    r.insert(m_vertex_to_selectable_map.at(vref));
            }
        }
        chunk_id++;
    }
    return r;
}

void Canvas::set_selection_mode(SelectionMode mode)
{
    m_selection_mode = mode;
    if (m_selection_mode == SelectionMode::HOVER || m_selection_mode == SelectionMode::HOVER_ONLY) {
        if (m_hover_selection.has_value())
            set_selection({m_hover_selection.value()}, true);
        else
            set_selection({}, true);
    }
    else if (m_selection_mode == SelectionMode::NONE) {
        clear_flags(VertexFlags::SELECTED | VertexFlags::HOVER);
        m_push_flags =
                static_cast<PushFlags>(m_push_flags | PF_LINES | PF_GLYPHS | PF_GLYPHS_3D | PF_ICONS | PF_PICTURES);
        queue_draw();
    }
    m_signal_selection_mode_changed.emit();
}

void Canvas::set_appearance(const Appearance &appearance)
{
    m_appearance = appearance;
    m_needs_resize = true;
    queue_draw();
}

void Canvas::set_clipping_planes(const ClippingPlanes &planes)
{
    m_clipping_planes = planes;
    queue_draw();
}

void Canvas::set_projection(Projection proj)
{
    m_projection = proj;
    queue_draw();
    m_signal_view_changed.emit();
}

static const float zoom_base = 1.5;

static float cam_dist_to_anim(float d)
{
    return log(d) / log(zoom_base);
}

static float cam_dist_from_anim(float d)
{
    return pow(zoom_base, d);
}


int Canvas::animate_step(GdkFrameClock *frame_clock)
{
    bool stop = true;
    for (auto anim : m_animators) {
        if (anim->step(gdk_frame_clock_get_frame_time(frame_clock) / 1e6))
            stop = false;
    }

    set_cam_quat(glm::quat(m_quat_w_animator.get_s(), m_quat_x_animator.get_s(), m_quat_y_animator.get_s(),
                           m_quat_z_animator.get_s()));
    const auto ca = glm::vec3{m_cx_animator.get_s_delta(), m_cy_animator.get_s_delta(), m_cz_animator.get_s_delta()};
    set_cam_distance(cam_dist_from_anim(m_zoom_animator.get_s()), m_animation_zoom_center);
    set_center(get_center() + ca);

    if (stop)
        return G_SOURCE_REMOVE;
    else
        return G_SOURCE_CONTINUE;
}

int Canvas::anim_tick_cb(GtkWidget *cwidget, GdkFrameClock *frame_clock, gpointer user_data)
{
    Gtk::Widget *widget = Glib::wrap(cwidget);
    auto canvas = dynamic_cast<Canvas *>(widget);
    return canvas->animate_step(frame_clock);
}


void Canvas::start_anim()
{
    const bool was_stopped =
            !std::any_of(m_animators.begin(), m_animators.end(), [](auto x) { return x->is_running(); });

    if (!m_quat_w_animator.is_running())
        m_quat_w_animator.start(m_cam_quat.w);
    if (!m_quat_x_animator.is_running())
        m_quat_x_animator.start(m_cam_quat.x);
    if (!m_quat_y_animator.is_running())
        m_quat_y_animator.start(m_cam_quat.y);
    if (!m_quat_z_animator.is_running())
        m_quat_z_animator.start(m_cam_quat.z);

    if (!m_zoom_animator.is_running())
        m_zoom_animator.start(cam_dist_to_anim(m_cam_distance));

    if (!m_cx_animator.is_running())
        m_cx_animator.start(m_center.x);
    if (!m_cy_animator.is_running())
        m_cy_animator.start(m_center.y);
    if (!m_cz_animator.is_running())
        m_cz_animator.start(m_center.z);

    if (was_stopped)
        gtk_widget_add_tick_callback(GTK_WIDGET(gobj()), &Canvas::anim_tick_cb, nullptr, nullptr);
}

void Canvas::update_bbox()
{
    MinMaxAccumulator<float> acc_x, acc_y, acc_z;
    for (const auto &chunk : m_chunks) {
        for (const auto &li : chunk.m_lines) {
            if ((li.flags & VertexFlags::SCREEN) != VertexFlags::DEFAULT)
                continue;
            acc_x.accumulate(li.x1);
            acc_x.accumulate(li.x2);
            acc_y.accumulate(li.y1);
            acc_y.accumulate(li.y2);
            acc_z.accumulate(li.z1);
            acc_z.accumulate(li.z2);
        }
        for (const auto &fv : chunk.m_face_vertex_buffer) {
            acc_x.accumulate(fv.x);
            acc_y.accumulate(fv.y);
            acc_z.accumulate(fv.z);
        }
    }
    m_bbox.first = {acc_x.get_min(), acc_y.get_min(), acc_z.get_min()};
    m_bbox.second = {acc_x.get_max(), acc_y.get_max(), acc_z.get_max()};
}

void Canvas::set_show_error_overlay(bool show)
{
    m_show_error_overlay = show;
    queue_draw();
}


void Canvas::set_override_selectable(const SelectableRef &sr)
{
    if (m_override_selectable_count == 0)
        m_override_selectable = sr;
    m_override_selectable_count++;
}

void Canvas::unset_override_selectable()
{
    if (m_override_selectable_count)
        m_override_selectable_count--;
    if (m_override_selectable_count == 0)
        m_override_selectable.reset();
}

glm::vec3 Canvas::transform_point(glm::vec3 p) const
{
    auto r = m_state.transform * glm::vec4(p, 1);
    return r;
}

glm::vec3 Canvas::transform_point_rel(glm::vec3 p) const
{
    auto r = m_state.transform * glm::vec4(p, 0);
    return r;
}

void Canvas::set_transform(const glm::mat4 &transform)
{
    m_state.transform = transform;
}

void Canvas::save()
{
    m_states.push_back(m_state);
}

void Canvas::restore()
{
    if (m_states.size() == 0)
        throw std::runtime_error("restore from empty states");

    m_state = m_states.back();
    m_states.pop_back();
}

} // namespace dune3d

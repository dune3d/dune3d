#include "canvas.hpp"
#include "selectable_ref.hpp"
#include "gl_util.hpp"
#include "import_step/import.hpp"
#include "bitmap_font_util.hpp"
#include "icon_texture_map.hpp"
#include "util/min_max_accumulator.hpp"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/io.hpp>
#include <fstream>


namespace dune3d {

Canvas::Canvas()
    : m_background_renderer(*this), m_face_renderer(*this), m_point_renderer(*this), m_line_renderer(*this),
      m_glyph_renderer(*this), m_icon_renderer(*this), m_box_selection(*this)
{
    set_can_focus(true);
    set_focusable(true);
    const MSD::Params slow_params{
            .mass = .0123,
            .damping = .2020,
            .springyness = 0.986,
    };
    for (auto anim : {&m_azimuth_animator, &m_elevation_animator, &m_cx_animator, &m_cy_animator, &m_cz_animator}) {
        anim->set_params(slow_params);
    }

    m_animators.push_back(&m_azimuth_animator);
    m_animators.push_back(&m_elevation_animator);
    m_animators.push_back(&m_zoom_animator);
    m_animators.push_back(&m_cx_animator);
    m_animators.push_back(&m_cy_animator);
    m_animators.push_back(&m_cz_animator);
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
        });
        controller->signal_released().connect([this](int n_press, double x, double y) { handle_click_release(); });
        controller->signal_cancel().connect([this](auto seq) { handle_click_release(); });
        add_controller(controller);
    }


    {
        auto controller = Gtk::GestureClick::create();
        controller->set_button(0);
        controller->signal_pressed().connect([this, controller](int n_press, double x, double y) {
            const auto shift = static_cast<bool>(controller->get_current_event_state() & Gdk::ModifierType::SHIFT_MASK);
            const auto button = controller->get_current_button();
            if (button == 2 || button == 3) {
                m_pointer_pos_orig = {x, y};
                if (shift == (button == 2)) {
                    m_pan_mode = PanMode::ROTATE;
                    m_cam_elevation_orig = m_cam_elevation;
                    m_cam_azimuth_orig = m_cam_azimuth;
                }
                else {
                    m_pan_mode = PanMode::MOVE;
                    m_center_orig = m_center;
                }
            }
        });
        controller->signal_released().connect([this](int n_press, double x, double y) { end_pan(); });
        add_controller(controller);
    }

    auto controller = Gtk::EventControllerMotion::create();
    controller->signal_motion().connect([this](double x, double y) {
        grab_focus();
        m_last_x = x;
        m_last_y = y;
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
                for (auto &x : m_lines) {
                    x.flags &= ~mask;
                }
                for (auto &x : m_points) {
                    x.flags &= ~mask;
                }
                for (auto &x : m_glyphs) {
                    x.flags &= ~mask;
                }
                for (auto &x : m_face_groups) {
                    x.flags &= ~mask;
                }
                for (auto &x : m_icons) {
                    x.flags &= ~mask;
                }
                update_drag_selection({x, y});
                return;
            }
        }
        const auto delta = glm::mat2(1, 0, 0, -1) * (glm::vec2(x, y) - m_pointer_pos_orig);
        if (m_pan_mode == PanMode::ROTATE) {
            set_cam_azimuth(m_cam_azimuth_orig - (delta.x / m_width) * 360);
            set_cam_elevation(m_cam_elevation_orig - (delta.y / m_height) * 90);
        }
        else if (m_pan_mode == PanMode::MOVE) {
            m_center = m_center_orig + get_center_shift(delta);
            queue_draw();
        }
        else {
            m_cursor_pos.x = (x / m_width) * 2. - 1.;
            m_cursor_pos.y = (y / m_height) * -2. + 1.;

            update_hover_selection();
            m_signal_cursor_moved.emit();
        }
    });
    add_controller(controller);
}

void Canvas::handle_click_release()
{
    m_dragging = false;
    m_inhibit_drag_selection = false;
    if (m_last_selection_mode == SelectionMode::NONE || m_last_selection_mode == SelectionMode::HOVER_ONLY)
        return;
    if (m_selection_mode == SelectionMode::DRAG) {
        m_box_selection.set_active(false);
        set_selection_mode(SelectionMode::NORMAL);
        m_signal_selection_changed.emit();
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
            auto sel = get_selection();
            if (sel.contains(m_hover_selection.value()))
                sel.erase(m_hover_selection.value());
            else
                sel.insert(m_hover_selection.value());
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
            const auto pick = read_pick_buf(x, y);
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
    const float zoom_base = 1.5;
    if (m_enable_animations) {
        if (dy == 0)
            return;
        start_anim();
        m_zoom_animator.target += dy;
    }
    else {
        set_cam_distance(m_cam_distance * pow(zoom_base, dy));
    }
}

void Canvas::scroll_move(double dx, double dy, Gtk::EventController &ctrl)
{
    auto delta = glm::vec2(dx * -50, dy * 50);
    m_center += get_center_shift(delta);
    queue_draw();
}

void Canvas::scroll_rotate(double dx, double dy, Gtk::EventController &ctrl)
{
    auto delta = -glm::vec2(dx, dy);

    set_cam_azimuth(get_cam_azimuth() - delta.x * 9);
    set_cam_elevation(get_cam_elevation() - delta.y * 9);
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
    m_cam_distance = m_gesture_zoom_cam_dist_orig / delta;
    queue_draw();
}

void Canvas::rotate_gesture_begin_cb(Gdk::EventSequence *seq)
{
    if (m_pan_mode != PanMode::NONE) {
        m_gesture_rotate->set_state(Gtk::EventSequenceState::DENIED);
        return;
    }
    m_gesture_rotate_cam_azimuth_orig = m_cam_azimuth;
    m_gesture_rotate_cam_elevation_orig = m_cam_elevation;
    double cx, cy;
    m_gesture_rotate->get_bounding_box_center(cx, cy);
    m_gesture_rotate_pos_orig = glm::vec2(cx, cy);
    m_gesture_rotate->set_state(Gtk::EventSequenceState::NONE);
}

void Canvas::rotate_gesture_update_cb(Gdk::EventSequence *seq)
{
    auto delta = m_gesture_rotate->get_angle_delta();
    if (m_gesture_rotate_cam_elevation_orig < 0)
        delta *= -1;
    set_cam_azimuth(m_gesture_rotate_cam_azimuth_orig + glm::degrees(delta));
    double cx, cy;
    m_gesture_rotate->get_bounding_box_center(cx, cy);
    auto dy = cy - m_gesture_rotate_pos_orig.y;
    set_cam_elevation(m_gesture_rotate_cam_elevation_orig + (dy / m_dev_height) * 180);
}

static float wrap_cam_elevation(float cam_elevation)
{
    while (cam_elevation >= 360)
        cam_elevation -= 360;
    while (cam_elevation < 0)
        cam_elevation += 360;
    if (cam_elevation > 180)
        cam_elevation -= 360;
    return cam_elevation;
}

static float wrap_cam_azimuth(float cam_azimuth)
{
    while (cam_azimuth < 0)
        cam_azimuth += 360;

    while (cam_azimuth > 360)
        cam_azimuth -= 360;
    return cam_azimuth;
}


void Canvas::set_cam_elevation(float ele)
{
    m_cam_elevation = wrap_cam_elevation(ele);
    queue_draw();
    m_signal_view_changed.emit();
}

void Canvas::set_cam_azimuth(float az)
{
    m_cam_azimuth = wrap_cam_azimuth(az);
    queue_draw();
    m_signal_view_changed.emit();
}

void Canvas::set_cam_distance(float dist)
{
    m_cam_distance = dist;
    queue_draw();
    m_signal_view_changed.emit();
}

void Canvas::set_center(glm::vec3 center)
{
    m_center = center;
    queue_draw();
    m_signal_view_changed.emit();
}

static double get_angle_delta(double d)
{
    while (d > 180)
        d -= 360;
    while (d < -180)
        d += 360;
    return d;
}


void Canvas::animate_to_azimuth_elevation_abs(float az, float el)
{
    animate_to_azimuth_elevation_rel(get_angle_delta(az - m_cam_azimuth), get_angle_delta(el - m_cam_elevation));
}

void Canvas::animate_to_azimuth_elevation_rel(float az, float el)
{
    if (!m_enable_animations) {
        set_cam_azimuth(get_cam_azimuth() + az);
        set_cam_elevation(get_cam_elevation() + el);
        return;
    }
    start_anim();

    m_azimuth_animator.target += az;
    m_elevation_animator.target += el;
}

void Canvas::animate_to_center_abs(const glm::vec3 &center)
{
    if (!m_enable_animations) {
        set_center(center);
        return;
    }
    start_anim();

    m_cx_animator.target = center.x;
    m_cy_animator.target = center.y;
    m_cz_animator.target = center.z;
}

Canvas::VertexRef Canvas::get_vertex_ref_for_pick(unsigned int pick) const
{
    for (const auto &[ty, it] : m_vertex_type_picks) {
        if ((pick >= it.offset) && ((pick - it.offset) < it.count))
            return {ty, pick - it.offset};
    }
    throw std::runtime_error("pick not found");
}

std::optional<SelectableRef> Canvas::get_selectable_ref_for_vertex_ref(const VertexRef &vref) const
{
    if (m_vertex_to_selectable_map.contains(vref))
        return m_vertex_to_selectable_map.at(vref);
    else
        return {};
}

std::optional<SelectableRef> Canvas::get_selectable_ref_for_pick(unsigned int pick) const
{
    if (!pick)
        return {};
    return get_selectable_ref_for_vertex_ref(get_vertex_ref_for_pick(pick));
}


void Canvas::update_hover_selection()
{
    if (m_vertex_type_picks.size() == 0)
        return;
    if (m_selection_mode != SelectionMode::NONE) {
        auto last_hover_selection = m_hover_selection;
        m_hover_selection.reset();
        auto pick = read_pick_buf(m_last_x, m_last_y);
        if (!pick || get_vertex_ref_for_pick(pick).type == VertexType::FACE_GROUP) {
            int box_size = 10;
            float best_distance = glm::vec2(box_size, box_size).length();
            unsigned int best_pick = pick;
            for (int dx = -box_size; dx <= box_size; dx++) {
                for (int dy = -box_size; dy <= box_size; dy++) {
                    int px = m_last_x + dx;
                    int py = m_last_y + dy;
                    if (px >= 0 && px < m_dev_width && py >= 0 && py < m_dev_height) {
                        if (auto p = read_pick_buf(px, py)) {
                            if (get_vertex_ref_for_pick(p).type == VertexType::FACE_GROUP)
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


        if (auto sr = get_selectable_ref_for_pick(pick))
            m_hover_selection = *sr;


        if (m_hover_selection != last_hover_selection) {
            auto mask = VertexFlags::HOVER;
            if (m_selection_mode == SelectionMode::HOVER || m_selection_mode == SelectionMode::HOVER_ONLY) {
                mask |= VertexFlags::SELECTED;
            }
            for (auto &x : m_lines) {
                x.flags &= ~mask;
            }
            for (auto &x : m_points) {
                x.flags &= ~mask;
            }
            for (auto &x : m_glyphs) {
                x.flags &= ~mask;
            }
            for (auto &x : m_face_groups) {
                x.flags &= ~mask;
            }
            for (auto &x : m_icons) {
                x.flags &= ~mask;
            }
            if (m_hover_selection.has_value()) {
                for (const auto &vref : m_selectable_to_vertex_map.at(m_hover_selection.value())) {
                    auto &flags = get_vertex_flags(vref);
                    flags |= mask;
                }
            }
            m_push_flags = static_cast<PushFlags>(m_push_flags | PF_LINES | PF_POINTS | PF_GLYPHS | PF_ICONS);
            queue_draw();
            m_signal_hover_selection_changed.emit();
        }
    }
}

uint16_t Canvas::read_pick_buf(int x, int y) const
{
    int xi = x * m_scale_factor;
    int yi = y * m_scale_factor;
    if (xi >= m_dev_width || yi >= m_dev_height || x < 0 || y < 0)
        return 0;
    const int idx = ((m_dev_height)-yi - 1) * m_dev_width + xi;
    return m_pick_buf.at(idx);
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

glm::vec3 Canvas::get_center_shift(const glm::vec2 &shift) const
{
    const float m = 0.1218f * m_cam_distance / 105.f;
    return {glm::rotate(glm::mat2(1, 0, 0, sin(glm::radians(m_cam_elevation))) * shift * m,
                        glm::radians(m_cam_azimuth - 90)),
            -cos(glm::radians(m_cam_elevation)) * shift.y * m};
}


void Canvas::on_realize()
{
    std::cout << "realize" << std::endl;
    Gtk::GLArea::on_realize();
    make_current();
    m_background_renderer.realize();
    m_face_renderer.realize();
    m_point_renderer.realize();
    m_line_renderer.realize();
    m_glyph_renderer.realize();
    m_icon_renderer.realize();
    m_box_selection.realize();
    GL_CHECK_ERROR


    glEnable(GL_DEPTH_TEST);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);

    GLint fb;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fb); // save fb

    glGenRenderbuffers(1, &m_renderbuffer);
    glGenRenderbuffers(1, &m_depthrenderbuffer);
    glGenRenderbuffers(1, &m_pickrenderbuffer);
    glGenRenderbuffers(1, &m_pickrenderbuffer_downsampled);

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

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_renderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, m_pickrenderbuffer);
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

void Canvas::resize_buffers()
{
    GLint rb;
    GLint samples = m_appearance.msaa;

    glGetIntegerv(GL_RENDERBUFFER_BINDING, &rb); // save rb
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, m_dev_width, m_dev_height);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthrenderbuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, m_dev_width, m_dev_height);
    glBindRenderbuffer(GL_RENDERBUFFER, m_pickrenderbuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_R16UI, m_dev_width, m_dev_height);

    glBindRenderbuffer(GL_RENDERBUFFER, m_pickrenderbuffer_downsampled);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_R16UI, m_dev_width, m_dev_height);

    glBindRenderbuffer(GL_RENDERBUFFER, rb);
}

ICanvas::VertexRef Canvas::add_face_group(const face::Faces &faces, glm::vec3 origin, glm::quat normal,
                                          FaceColor face_color)
{
    auto offset = m_face_index_buffer.size();
    add_faces(faces);
    auto length = m_face_index_buffer.size() - offset;
    m_face_groups.push_back(FaceGroup{
            .offset = offset,
            .length = length,
            .origin = origin,
            .normal = normal,
            .color = face_color,
    });

    return {VertexType::FACE_GROUP, m_face_groups.size() - 1};
}

void Canvas::add_faces(const face::Faces &faces)
{
    size_t vertex_offset = m_face_vertex_buffer.size();
    for (const auto &face : faces) {
        for (size_t i = 0; i < face.vertices.size(); i++) {
            const auto &v = face.vertices.at(i);
            const auto &n = face.normals.at(i);
            m_face_vertex_buffer.emplace_back(v.x, v.y, v.z, n.x, n.y, n.z, face.color.r * 255, face.color.g * 255,
                                              face.color.b * 255);
        }

        for (const auto &tri : face.triangle_indices) {
            size_t a, b, c;
            std::tie(a, b, c) = tri;
            m_face_index_buffer.push_back(a + vertex_offset);
            m_face_index_buffer.push_back(b + vertex_offset);
            m_face_index_buffer.push_back(c + vertex_offset);
        }
        vertex_offset += face.vertices.size();
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

    GLint fb;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fb); // save fb

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glClearColor(0, 0, 0, 0);
    glClearDepth(10);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GL_CHECK_ERROR
    {
        const std::array<GLenum, 2> bufs = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(bufs.size(), bufs.data());
    }

    glDisable(GL_DEPTH_TEST);
    m_background_renderer.render();
    glEnable(GL_DEPTH_TEST);


    if (m_push_flags & PF_FACES)
        m_face_renderer.push();
    if (m_push_flags & PF_POINTS)
        m_point_renderer.push();
    if (m_push_flags & PF_LINES)
        m_line_renderer.push();
    if (m_push_flags & PF_GLYPHS)
        m_glyph_renderer.push();
    if (m_push_flags & PF_ICONS)
        m_icon_renderer.push();

    m_push_flags = PF_NONE;

    {
        float r = m_cam_distance;
        float phi = glm::radians(m_cam_azimuth);
        float theta = glm::radians(90 - m_cam_elevation);
        auto cam_offset = glm::vec3(r * sin(theta) * cos(phi), r * sin(theta) * sin(phi), r * cos(theta));
        auto cam_pos = cam_offset + m_center;

        glm::vec3 right(sin(phi - 3.14f / 2.0f), cos(phi - 3.14f / 2.0f), 0);

        m_viewmat = glm::lookAt(cam_pos, m_center, glm::vec3(0, 0, std::abs(m_cam_elevation) < 90 ? 1 : -1));

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

        float m = tan(0.5 * glm::radians(m_cam_fov)) / m_dev_height * m_cam_distance;
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

    m_pick_base = 1;
    m_face_renderer.render();
    GL_CHECK_ERROR
    // glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    m_point_renderer.render();
    m_line_renderer.render();
    glEnable(GL_BLEND);
    m_glyph_renderer.render();
    m_icon_renderer.render();
    glDisable(GL_DEPTH_TEST);
    m_box_selection.render();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
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
    m_pick_buf.resize(m_dev_width * m_dev_height);
    GL_CHECK_ERROR

    glPixelStorei(GL_PACK_ALIGNMENT, 2);
    glReadPixels(0, 0, m_dev_width, m_dev_height, GL_RED_INTEGER, GL_UNSIGNED_SHORT, m_pick_buf.data());

    GL_CHECK_ERROR
    if (m_pick_state == PickState::QUEUED) {
        m_pick_state = PickState::CURRENT;
        std::ofstream ofs("/tmp/pick.txt");
        for (int y = 0; y < m_dev_height; y++) {
            for (int x = 0; x < m_dev_width; x++) {
                ofs << m_pick_buf.at(x + y * m_dev_width) << " ";
            }
            ofs << std::endl;
        }
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
    glDrawBuffer(fb ? GL_COLOR_ATTACHMENT0 : GL_FRONT);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, m_dev_width, m_dev_height, 0, 0, m_dev_width, m_dev_height, GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    GL_CHECK_ERROR
    glFlush();

    if (first_render) {
        update_hover_selection();
    }

    return true;
}

void Canvas::on_resize(int width, int height)
{
    if (width == m_dev_width && height == m_dev_height)
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
    m_scale_factor = get_scale_factor();

    Gtk::GLArea::on_resize(width, height);
}

void Canvas::request_push()
{
    m_push_flags = PF_ALL;
    queue_draw();
}

void Canvas::queue_pick()
{
    m_pick_state = PickState::QUEUED;
    queue_draw();
}

void Canvas::clear()
{
    std::cout << "clear" << std::endl;
    m_face_index_buffer.clear();
    m_face_vertex_buffer.clear();
    m_face_groups.clear();
    m_points.clear();
    m_points_selection_invisible.clear();
    m_lines.clear();
    m_lines_selection_invisible.clear();
    m_glyphs.clear();
    m_icons.clear();
    m_selectable_to_vertex_map.clear();
    m_vertex_to_selectable_map.clear();
    m_vertex_type_picks.clear();
    m_push_flags = PF_ALL;
    queue_draw();
}

ICanvas::VertexRef Canvas::draw_point(glm::vec3 p)
{
    auto &pts = m_selection_invisible ? m_points_selection_invisible : m_points;
    auto &pt = pts.emplace_back(p);
    apply_flags(pt.flags);
    if (m_selection_invisible)
        return {VertexType::SELECTION_INVISIBLE, 0};
    return {VertexType::POINT, m_points.size() - 1};
}

ICanvas::VertexRef Canvas::draw_line(glm::vec3 a, glm::vec3 b)
{
    auto &lines = m_selection_invisible ? m_lines_selection_invisible : m_lines;
    auto &li = lines.emplace_back(a, b);
    apply_flags(li.flags);

    if (m_selection_invisible)
        return {VertexType::SELECTION_INVISIBLE, 0};
    return {VertexType::LINE, m_lines.size() - 1};
}

ICanvas::VertexRef Canvas::draw_screen_line(glm::vec3 a, glm::vec3 b)
{
    auto &lines = m_selection_invisible ? m_lines_selection_invisible : m_lines;

    auto &li = lines.emplace_back(a, b);
    li.flags |= VertexFlags::SCREEN;
    apply_flags(li.flags);
    if (m_selection_invisible)
        return {VertexType::SELECTION_INVISIBLE, 0};
    return {VertexType::LINE, m_lines.size() - 1};
}

void Canvas::apply_flags(VertexFlags &flags)
{
    if (m_vertex_inactive)
        flags |= VertexFlags::INACTIVE;
    if (m_vertex_constraint)
        flags |= VertexFlags::CONSTRAINT;
    if (m_vertex_construction)
        flags |= VertexFlags::CONSTRUCTION;
}

static const float char_space = 1;


std::vector<ICanvas::VertexRef> Canvas::draw_bitmap_text(const glm::vec3 p, float size, const std::string &rtext,
                                                         int angle)
{
    std::vector<ICanvas::VertexRef> vrefs;
    Glib::ustring text(rtext);
    auto smooth_px = bitmap_font::get_smooth_pixels();

    float sc = size * .75;

    glm::vec2 point = {0, 0};

    glm::vec2 v = {1, 0};
    for (auto codepoint : text) {
        if (codepoint != ' ') {
            auto info = bitmap_font::get_glyph_info(codepoint);
            if (!info.is_valid()) {
                info = bitmap_font::get_glyph_info('?');
            }

            unsigned int glyph_x = info.atlas_x + smooth_px;
            unsigned int glyph_y = info.atlas_y + smooth_px;
            unsigned int glyph_w = info.atlas_w - smooth_px * 2;
            unsigned int glyph_h = info.atlas_h - smooth_px * 2;

            uint32_t bits =
                    (glyph_h & 0x3f) | ((glyph_w & 0x3f) << 6) | ((glyph_y & 0x3ff) << 12) | ((glyph_x & 0x3ff) << 22);

            glm::vec2 shift(info.minx, -info.miny);

            auto ps = point + shift * sc;

            auto &gl = m_glyphs.emplace_back(p.x, p.y, p.z, ps.x, ps.y, sc, bits);
            apply_flags(gl.flags);


            vrefs.push_back({VertexType::GLYPH, m_glyphs.size() - 1});

            // add_triangle(layer, point + tr.transform(shift) * 1e6 * sc, v * (glyph_w * 1e6 * sc), Coordf(aspect,
            // *fl),
            //              color, TriangleInfo::FLAG_GLYPH);
            point += v * (info.advance * char_space * sc);
        }
        else {
            point += v * (7 * char_space * sc);
        }
    }


    return vrefs;
}

ICanvas::VertexRef Canvas::draw_icon(IconTexture::IconTextureID id, glm::vec3 origin, glm::vec2 shift)
{
    auto icon_pos = IconTexture::icon_texture_map.at(id);
    auto &icon = m_icons.emplace_back(origin.x, origin.y, origin.z, shift.x, shift.y, icon_pos.x, icon_pos.y);
    apply_flags(icon.flags);
    return {VertexType::ICON, m_icons.size() - 1};
}


void Canvas::add_selectable(const VertexRef &vref, const SelectableRef &sref)
{
    if (vref.type == VertexType::SELECTION_INVISIBLE)
        return;
    m_vertex_to_selectable_map.emplace(vref, sref);
    m_selectable_to_vertex_map[sref].push_back(vref);
}

Canvas::VertexFlags &Canvas::get_vertex_flags(const VertexRef &vref)
{
    switch (vref.type) {
    case VertexType::LINE:
        return m_lines.at(vref.index).flags;

    case VertexType::POINT:
        return m_points.at(vref.index).flags;

    case VertexType::GLYPH:
        return m_glyphs.at(vref.index).flags;

    case VertexType::FACE_GROUP:
        return m_face_groups.at(vref.index).flags;

    case VertexType::ICON:
        return m_icons.at(vref.index).flags;

    default:
        throw std::runtime_error("unknown vertex type");
    }
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
    for (auto &x : m_lines) {
        x.flags &= ~flag;
    }
    for (auto &x : m_points) {
        x.flags &= ~flag;
    }
    for (auto &x : m_glyphs) {
        x.flags &= ~flag;
    }
    for (auto &x : m_face_groups) {
        x.flags &= ~flag;
    }
    for (auto &x : m_icons) {
        x.flags &= ~flag;
    }
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
    m_push_flags = static_cast<PushFlags>(m_push_flags | PF_LINES | PF_POINTS | PF_GLYPHS | PF_ICONS);
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
    for (size_t i = 0; i < m_lines.size(); i++) {
        if ((m_lines.at(i).flags & VertexFlags::SELECTED) != VertexFlags::DEFAULT) {
            const VertexRef vref{.type = VertexType::LINE, .index = i};
            if (m_vertex_to_selectable_map.count(vref))
                r.insert(m_vertex_to_selectable_map.at(vref));
        }
    }
    for (size_t i = 0; i < m_points.size(); i++) {
        if ((m_points.at(i).flags & VertexFlags::SELECTED) != VertexFlags::DEFAULT) {
            const VertexRef vref{.type = VertexType::POINT, .index = i};
            if (m_vertex_to_selectable_map.count(vref))
                r.insert(m_vertex_to_selectable_map.at(vref));
        }
    }
    for (size_t i = 0; i < m_glyphs.size(); i++) {
        if ((m_glyphs.at(i).flags & VertexFlags::SELECTED) != VertexFlags::DEFAULT) {
            const VertexRef vref{.type = VertexType::GLYPH, .index = i};
            if (m_vertex_to_selectable_map.count(vref))
                r.insert(m_vertex_to_selectable_map.at(vref));
        }
    }
    for (size_t i = 0; i < m_face_groups.size(); i++) {
        if ((m_face_groups.at(i).flags & VertexFlags::SELECTED) != VertexFlags::DEFAULT) {
            const VertexRef vref{.type = VertexType::FACE_GROUP, .index = i};
            if (m_vertex_to_selectable_map.count(vref))
                r.insert(m_vertex_to_selectable_map.at(vref));
        }
    }
    for (size_t i = 0; i < m_icons.size(); i++) {
        if ((m_icons.at(i).flags & VertexFlags::SELECTED) != VertexFlags::DEFAULT) {
            const VertexRef vref{.type = VertexType::ICON, .index = i};
            if (m_vertex_to_selectable_map.count(vref))
                r.insert(m_vertex_to_selectable_map.at(vref));
        }
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
        for (auto &x : m_lines) {
            x.flags &= ~(VertexFlags::SELECTED | VertexFlags::HOVER);
        }
        for (auto &x : m_points) {
            x.flags &= ~(VertexFlags::SELECTED | VertexFlags::HOVER);
        }
        for (auto &x : m_glyphs) {
            x.flags &= ~(VertexFlags::SELECTED | VertexFlags::HOVER);
        }
        for (auto &x : m_face_groups) {
            x.flags &= ~(VertexFlags::SELECTED | VertexFlags::HOVER);
        }
        for (auto &x : m_icons) {
            x.flags &= ~(VertexFlags::SELECTED | VertexFlags::HOVER);
        }
        m_push_flags = static_cast<PushFlags>(m_push_flags | PF_LINES | PF_POINTS | PF_GLYPHS | PF_ICONS);
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

void Canvas::set_projection(Projection proj)
{
    m_projection = proj;
    queue_draw();
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

    set_cam_azimuth(m_azimuth_animator.get_s());
    set_cam_elevation(m_elevation_animator.get_s());
    set_cam_distance(cam_dist_from_anim(m_zoom_animator.get_s()));
    set_center({m_cx_animator.get_s(), m_cy_animator.get_s(), m_cz_animator.get_s()});

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

    if (!m_azimuth_animator.is_running())
        m_azimuth_animator.start(m_cam_azimuth);

    if (!m_elevation_animator.is_running())
        m_elevation_animator.start(m_cam_elevation);

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
    for (const auto &li : m_lines) {
        acc_x.accumulate(li.x1);
        acc_x.accumulate(li.x2);
        acc_y.accumulate(li.y1);
        acc_y.accumulate(li.y2);
        acc_z.accumulate(li.z1);
        acc_z.accumulate(li.z2);
    }
    for (const auto &fv : m_face_vertex_buffer) {
        acc_x.accumulate(fv.x);
        acc_y.accumulate(fv.y);
        acc_z.accumulate(fv.z);
    }
    m_bbox.first = {acc_x.get_min(), acc_y.get_min(), acc_z.get_min()};
    m_bbox.second = {acc_x.get_max(), acc_y.get_max(), acc_z.get_max()};
}


} // namespace dune3d

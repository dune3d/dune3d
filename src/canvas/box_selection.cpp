#include "box_selection.hpp"
#include "canvas.hpp"
#include "gl_util.hpp"
#include "color_palette.hpp"
#include <glm/gtc/type_ptr.hpp>


namespace dune3d {

static GLuint create_vao(GLuint program)
{
    GLuint vao;

    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &vao);

    return vao;
}

void BoxSelection::realize()
{
    m_program = gl_create_program_from_resource(
            "/org/dune3d/dune3d/canvas/shaders/"
            "selection-vertex.glsl",
            "/org/dune3d/dune3d/canvas/shaders/"
            "selection-fragment.glsl",
            nullptr);
    m_vao = create_vao(m_program);

    GET_LOC(this, screenmat);
    GET_LOC(this, a);
    GET_LOC(this, b);
    GET_LOC(this, fill);
    GET_LOC(this, color);
}

void BoxSelection::set_active(bool active)
{
    m_active = active;
    m_ca.queue_draw();
}

void BoxSelection::set_box(glm::vec2 a, glm::dvec2 b)
{
    m_sel_a = a;
    m_sel_b = b;
    m_ca.queue_draw();
}


void BoxSelection::render()
{
    if (!m_active)
        return;
    glUseProgram(m_program);
    glBindVertexArray(m_vao);
    glUniformMatrix3fv(m_screenmat_loc, 1, GL_FALSE, glm::value_ptr(m_ca.m_screenmat));
    glUniform2f(m_a_loc, m_sel_a.x, m_sel_a.y);
    glUniform2f(m_b_loc, m_sel_b.x, m_sel_b.y);
    glUniform1i(m_fill_loc, 0);
    gl_color_to_uniform_3f(m_color_loc, m_ca.m_appearance.get_color(ColorP::SELECTION_BOX));

    glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glBindVertexArray(0);
    glUseProgram(0);
}
/*

void DragSelection::drag_begin(GdkEventButton *button_event)
{
    if (button_event->state & Gdk::SHIFT_MASK)
        return;
    if (!ca.selection_allowed)
        return;
    if (button_event->type == GDK_2BUTTON_PRESS) {
        active = 0;
        ca.drag_selection_inhibited = false;
        return;
    }
    gdouble x, y;
    gdk_event_get_coords((GdkEvent *)button_event, &x, &y);
    if (button_event->button == 1) { // inside of grid and middle mouse button
        active = 1;
        sel_o = Coordf(x, y);
        if (!is_line_sel(ca.selection_tool)) {
            box.sel_a = ca.screen2canvas(sel_o);
            box.sel_b = box.sel_a;
        }
        else {
            line.vertices.clear();
            auto c = ca.screen2canvas(sel_o);
            line.vertices.emplace_back(c.x, c.y);
            line.path.clear();
            line.path.emplace_back(c.x, c.y);
            line.update();
        }
        ca.queue_draw();
    }
}

void DragSelection::drag_move(GdkEventMotion *motion_event)
{
    gdouble x, y;
    gdk_event_get_coords((GdkEvent *)motion_event, &x, &y);
    if (ca.drag_selection_inhibited && active) {
        active = 0;
        return;
    }

    if (active == 1) {
        if (is_line_sel(ca.selection_tool)) {
            if (ABS(sel_o.x - x) > 10 || ABS(sel_o.y - y) > 10) {
                active = 2;
                ca.set_selection_mode(CanvasGL::SelectionMode::NORMAL);
            }
        }
        else {
            if (ABS(sel_o.x - x) > 10 && ABS(sel_o.y - y) > 10) {
                active = 2;
                ca.set_selection_mode(CanvasGL::SelectionMode::NORMAL);
            }
        }
    }
    else if (active == 2) {
        if (!is_line_sel(ca.selection_tool)) {
            box.sel_b = ca.screen2canvas(Coordf(x, y));
            box.update();
        }
        else {
            auto c = ca.screen2canvas(Coordf(x, y));
            line.vertices.emplace_back(c.x, c.y);
            line.path.emplace_back(c.x, c.y);
            line.update();
        }
        ca.queue_draw();
    }
}

void DragSelection::drag_end(GdkEventButton *button_event)
{
    if (button_event->button == 1) { // inside of grid and middle mouse button {
        const bool mod = (button_event->state & Gdk::CONTROL_MASK) || ca.selection_sticky;
        if (active == 2) {
            for (auto &it : ca.selectables.items) {
                if (it.get_flag(Selectable::Flag::PRELIGHT)) {
                    if (mod) {
                        switch (ca.selection_modifier_action) {
                        case CanvasGL::SelectionModifierAction::TOGGLE:
                            it.set_flag(Selectable::Flag::SELECTED, !it.get_flag(Selectable::Flag::SELECTED));
                            break;
                        case CanvasGL::SelectionModifierAction::ADD:
                            it.set_flag(Selectable::Flag::SELECTED, true);
                            break;
                        case CanvasGL::SelectionModifierAction::REMOVE:
                            it.set_flag(Selectable::Flag::SELECTED, false);
                            break;
                        }
                    }
                    else {
                        it.set_flag(Selectable::Flag::SELECTED, true);
                    }
                }
                else {
                    if (!mod)
                        it.set_flag(Selectable::Flag::SELECTED, false);
                }
                it.set_flag(Selectable::Flag::PRELIGHT, false);
            }
            ca.selectables.update_preview(ca.get_selection());
            ca.request_push(CanvasGL::PF_DRAG_SELECTION);
            ca.request_push(CanvasGL::PF_SELECTABLES);
            ca.s_signal_selection_changed.emit();
        }
        else if (active == 1) {
            std::cout << "click select" << std::endl;
            if (ca.selection_mode == CanvasGL::SelectionMode::HOVER) { // just select what was
                                                                       // selecte by hover select
                auto sel = ca.get_selection();
                if (sel.size()) {
                    ca.set_selection_mode(CanvasGL::SelectionMode::NORMAL);
                    ca.selectables.update_preview(sel);
                    ca.s_signal_selection_changed.emit();
                }
            }
            else {
                std::set<SelectableRef> selection;
                selection = ca.get_selection();
                gdouble x, y;
                gdk_event_get_coords((GdkEvent *)button_event, &x, &y);
                auto c = ca.screen2canvas({(float)x, (float)y});
                for (auto &it : ca.selectables.items) {
                    it.set_flag(Selectable::Flag::PRELIGHT, false);
                    it.set_flag(Selectable::Flag::SELECTED, false);
                }
                auto sel_from_canvas = ca.get_selection_at(Coordi(c.x, c.y));

                if (sel_from_canvas.size() > 1) {
                    ca.set_selection(selection, false);
                    for (const auto it : ca.clarify_menu->get_children()) {
                        ca.clarify_menu->remove(*it);
                    }
                    for (const auto sr : sel_from_canvas) {
                        auto text = ca.s_signal_request_display_name.emit(sr);
                        Gtk::MenuItem *la = nullptr;
                        if (mod) {
                            auto l = Gtk::manage(new Gtk::CheckMenuItem(text));
                            l->set_active(selection.count(sr));
                            la = l;
                        }
                        else {
                            la = Gtk::manage(new Gtk::MenuItem(text));
                        }
                        la->signal_select().connect([this, selection, sr, mod] {
                            auto sel = selection;
                            if (mod) {
                                if (sel.count(sr)) {
                                    sel.erase(sr);
                                }
                                else {
                                    sel.insert(sr);
                                }
                                ca.set_selection(sel);
                            }
                            else {
                                ca.set_selection({sr}, false);
                            }
                        });
                        la->signal_deselect().connect([this, selection, mod] {
                            if (mod) {
                                ca.set_selection(selection, false);
                            }
                            else {
                                ca.set_selection({}, false);
                            }
                        });
                        la->signal_activate().connect([this, sr, selection, mod] {
                            auto sel = selection;
                            if (mod) {
                                if (sel.count(sr)) {
                                    sel.erase(sr);
                                }
                                else {
                                    sel.insert(sr);
                                }
                                ca.set_selection(sel);
                            }
                            else {
                                ca.set_selection({sr}, true);
                            }
                        });
                        la->show();
                        ca.clarify_menu->append(*la);
                    }
#if GTK_CHECK_VERSION(3, 22, 0)
                    ca.clarify_menu->popup_at_pointer((GdkEvent *)button_event);
#else
                    ca.clarify_menu->popup(0, gtk_get_current_event_time());
#endif
                }
                else if (sel_from_canvas.size() == 1) {
                    auto sel = *sel_from_canvas.begin();
                    if (mod) {
                        switch (ca.selection_modifier_action) {
                        case CanvasGL::SelectionModifierAction::TOGGLE:
                            if (selection.count(sel)) {
                                selection.erase(sel);
                            }
                            else {
                                selection.insert(sel);
                            }
                            break;
                        case CanvasGL::SelectionModifierAction::ADD:
                            selection.insert(sel);
                            break;
                        case CanvasGL::SelectionModifierAction::REMOVE:
                            selection.erase(sel);
                            break;
                        }
                        ca.set_selection(selection);
                    }
                    else {
                        ca.set_selection({sel});
                    }
                }
                else if (sel_from_canvas.size() == 0) {
                    if (mod)
                        ca.set_selection(selection);
                    else
                        ca.set_selection({});
                }
            }
        }
        active = 0;
        ca.queue_draw();
        ca.drag_selection_inhibited = false;
    }
}
*/
/*
void DragSelection::Box::update()
{
    const auto sel_center = (sel_a + sel_b) / 2;
    const auto sel_a_screen = ca.canvas2screen(sel_a);
    const auto sel_b_screen = ca.canvas2screen(sel_b);
    const auto sel_width = std::abs(sel_b_screen.x - sel_a_screen.x) / ca.scale;
    const auto sel_height = std::abs(sel_b_screen.y - sel_a_screen.y) / ca.scale;
    const auto sel_angle = (ca.flip_view ? -1 : 1) * ca.view_angle;
    auto in_box = [sel_center, sel_width, sel_height, sel_angle](Coordf p) {
        p -= sel_center;
        p = p.rotate(sel_angle);
        return std::abs(p.x) < sel_width / 2 && std::abs(p.y) < sel_height / 2;
    };
    auto sq = ca.selection_qualifier;

    if (sq == CanvasGL::SelectionQualifier::AUTO) {
        if (sel_a_screen.x < sel_b_screen.x)
            sq = CanvasGL::SelectionQualifier::INCLUDE_BOX;
        else
            sq = CanvasGL::SelectionQualifier::TOUCH_BOX;
    }

    ClipperLib::Path clbox(4);
    if (sq == CanvasGL::SelectionQualifier::TOUCH_BOX) {
        const auto sz1 = Coordf(sel_width / 2, sel_height / 2).rotate(-sel_angle);
        const auto sz2 = Coordf(sel_width / 2, sel_height / -2).rotate(-sel_angle);
        clbox.at(0) = to_pt(sel_center + sz1);
        clbox.at(1) = to_pt(sel_center + sz2);
        clbox.at(2) = to_pt(sel_center - sz1);
        clbox.at(3) = to_pt(sel_center - sz2);
    }

    unsigned int i = 0;
    for (auto &it : ca.selectables.items) {
        it.set_flag(Selectable::Flag::PRELIGHT, false);
        if (ca.selection_filter.can_select(ca.selectables.items_ref[i])) {


            if (sq == CanvasGL::SelectionQualifier::INCLUDE_ORIGIN) {
                if (in_box({it.x, it.y})) {
                    it.set_flag(Selectable::Flag::PRELIGHT, true);
                }
                fill = true;
            }
            else if (sq == CanvasGL::SelectionQualifier::INCLUDE_BOX) {
                auto corners = path_from_sel(it);
                if (std::all_of(corners.begin(), corners.end(),
                                [in_box](const auto &a) { return in_box(Coordf(a.X, a.Y)); })) {
                    it.set_flag(Selectable::Flag::PRELIGHT, true);
                }
                fill = false;
            }
            else if (sq == CanvasGL::SelectionQualifier::TOUCH_BOX) {
                // possible optimisation: don't use clipper
                ClipperLib::Path sel = path_from_sel(it);

                ClipperLib::Clipper clipper;
                clipper.AddPath(clbox, ClipperLib::ptSubject, true);
                clipper.AddPath(sel, ClipperLib::ptClip, true);

                ClipperLib::Paths isect;
                clipper.Execute(ClipperLib::ctIntersection, isect);

                if (isect.size()) {
                    it.set_flag(Selectable::Flag::PRELIGHT, true);
                }
                fill = true;
            }
        }
        i++;
    }
    ca.request_push(CanvasGL::PF_DRAG_SELECTION);
    ca.request_push(CanvasGL::PF_SELECTABLES);
}

*/
} // namespace dune3d

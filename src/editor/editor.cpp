#include "editor.hpp"
#include "dune3d_appwindow.hpp"
#include "core/tool_id.hpp"
#include "widgets/constraints_box.hpp"
#include "action/action_id.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include "in_tool_action/in_tool_action_catalog.hpp"
#include "canvas/canvas.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_document.hpp"
#include "tool_popover.hpp"
#include "dune3d_application.hpp"
#include "util/selection_util.hpp"
#include "group_editor.hpp"
#include "render/renderer.hpp"
#include "document/entity/entity_workplane.hpp"
#include "logger/logger.hpp"
#include "document/constraint/constraint.hpp"
#include "util/fs_util.hpp"
#include "util/util.hpp"
#include "selection_editor.hpp"
#include "preferences/color_presets.hpp"
#include "workspace_browser.hpp"
#include "document/constraint/iconstraint_workplane.hpp"
#include "widgets/clipping_plane_window.hpp"
#include "widgets/selection_filter_window.hpp"
#include "system/system.hpp"
#include "logger/log_util.hpp"
#include "nlohmann/json.hpp"
#include "buffer.hpp"
#include <iostream>

namespace dune3d {
Editor::Editor(Dune3DAppWindow &win, Preferences &prefs)
    : m_preferences(prefs), m_dialogs(win, *this), m_win(win), m_core(*this), m_selection_menu_creator(m_core)
{
    m_drag_tool = ToolID::NONE;
}

Editor::~Editor() = default;

void Editor::init()
{
    init_workspace_browser();
    init_properties_notebook();
    init_header_bar();
    init_actions();
    init_tool_popover();
    init_canvas();

    m_core.signal_needs_save().connect([this] {
        update_action_sensitivity();
        m_workspace_browser->update_needs_save();
        update_workspace_view_names();
    });
    get_canvas().signal_selection_changed().connect([this] { update_action_sensitivity(); });

    m_win.signal_close_request().connect(
            [this] {
                if (!m_core.get_needs_save_any())
                    return false;

                auto cb = [this] {
                    // here, the close dialog is still there and closing the main window causes a near-segfault
                    // so break out of the current event
                    Glib::signal_idle().connect_once([this] { m_win.close(); });
                };
                close_document(m_core.get_current_idocument_info().get_uuid(), cb, cb);

                return true; // keep window open
            },
            true);


    update_workplane_label();


    m_preferences.signal_changed().connect(sigc::mem_fun(*this, &Editor::apply_preferences));

    m_core.signal_tool_changed().connect(sigc::mem_fun(*this, &Editor::handle_tool_change));


    m_core.signal_documents_changed().connect([this] {
        for (auto doc : m_core.get_documents()) {
            for (auto &[uu, wsv] : m_workspace_views) {
                wsv.m_documents[doc->get_uuid()];
            }
        }
        m_win.get_workspace_notebook().set_visible(m_core.has_documents());
        canvas_update_keep_selection();
        m_workspace_browser->update_documents(get_current_document_views());
        update_group_editor();
        update_workplane_label();
        update_action_sensitivity();
        m_workspace_browser->set_sensitive(m_core.has_documents());
        m_win.set_welcome_box_visible(!m_core.has_documents());
        update_version_info();
        update_action_bar_buttons_sensitivity();
        update_action_bar_visibility();
        update_selection_editor();
        update_title();
    });

    attach_action_button(m_win.get_welcome_open_button(), ActionID::OPEN_DOCUMENT);
    attach_action_button(m_win.get_welcome_new_button(), ActionID::NEW_DOCUMENT);

    create_action_bar_button(ToolID::DRAW_CONTOUR);
    create_action_bar_button(ToolID::DRAW_RECTANGLE);
    create_action_bar_button(ToolID::DRAW_CIRCLE_2D);
    create_action_bar_button(ToolID::DRAW_REGULAR_POLYGON);
    create_action_bar_button(ToolID::DRAW_TEXT);
    create_action_bar_button(ToolID::DRAW_WORKPLANE);

    init_view_options();

    m_clipping_plane_window = std::make_unique<ClippingPlaneWindow>();
    m_clipping_plane_window->set_transient_for(m_win);
    connect_action(ActionID::CLIPPING_PLANE_WINDOW, [this](const auto &a) { m_clipping_plane_window->present(); });
    connect_action(ActionID::TOGGLE_CLIPPING_PLANES,
                   [this](const auto &a) { m_clipping_plane_window->toggle_global(); });
    m_clipping_plane_window->signal_changed().connect([this] {
        get_canvas().set_clipping_planes(m_clipping_plane_window->get_planes());
        update_view_hints();
    });
    m_clipping_plane_window->set_hide_on_close(true);

    m_selection_filter_window = std::make_unique<SelectionFilterWindow>(m_core);
    m_selection_filter_window->set_transient_for(m_win);
    m_selection_filter_window->set_hide_on_close(true);
    connect_action(ActionID::SELECTION_FILTER, [this](const auto &a) { m_selection_filter_window->present(); });
    get_canvas().set_selection_filter(*m_selection_filter_window);
    m_selection_filter_window->signal_changed().connect(sigc::mem_fun(*this, &Editor::update_view_hints));

    connect_action(ActionID::SELECT_UNDERCONSTRAINED, [this](const auto &a) {
        auto &doc = m_core.get_current_document();
        System sys{doc, m_core.get_current_group()};
        std::set<EntityAndPoint> free_points;
        sys.solve(&free_points);
        std::set<SelectableRef> sel;
        for (const auto &enp : free_points) {
            sel.emplace(SelectableRef::Type::ENTITY, enp.entity, enp.point);
        }
        get_canvas().set_selection(sel, true);
        get_canvas().set_selection_mode(SelectionMode::NORMAL);
    });

    m_win.signal_undo().connect([this] { trigger_action(ActionID::UNDO); });

    update_action_sensitivity();
    reset_key_hint_label();

    m_win.get_workspace_add_button().signal_clicked().connect([this] {
        auto new_wv_uu = create_workspace_view_from_current();
        set_current_workspace_view(new_wv_uu);
    });

    m_win.get_canvas().signal_view_changed().connect([this] {
        if (!m_current_workspace_view)
            return;
        if (m_workspace_view_loading)
            return;
        auto &wv = m_workspace_views.at(m_current_workspace_view);
        auto &ca = m_win.get_canvas();
        wv.m_cam_distance = ca.get_cam_distance();
        wv.m_cam_quat = ca.get_cam_quat();
        wv.m_center = ca.get_center();
        wv.m_projection = ca.get_projection();
    });

    m_win.get_workspace_notebook().signal_switch_page().connect([this](Gtk::Widget *page, guint index) {
        auto &pg = dynamic_cast<WorkspaceViewPage &>(*page);
        set_current_workspace_view(pg.m_uuid);
    });

    apply_preferences();
}

void Editor::add_tool_action(ActionToolID id, const std::string &action)
{
    m_win.add_action(action, [this, id] { trigger_action(id); });
}

void Editor::init_view_options()
{
    auto view_options_popover = Gtk::make_managed<Gtk::PopoverMenu>();
    m_win.get_view_options_button().set_popover(*view_options_popover);
    {
        Gdk::Rectangle rect;
        rect.set_width(32);
        m_win.get_view_options_button().get_popover()->set_pointing_to(rect);
    }

    m_view_options_menu = Gio::Menu::create();
    m_perspective_action = m_win.add_action_bool("perspective", false);
    m_perspective_action->signal_change_state().connect([this](const Glib::VariantBase &v) {
        auto b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(v).get();
        set_perspective_projection(b);
    });
    m_previous_construction_entities_action = m_win.add_action_bool("previous_construction", false);
    m_previous_construction_entities_action->signal_change_state().connect([this](const Glib::VariantBase &v) {
        auto b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(v).get();
        set_show_previous_construction_entities(b);
    });

    add_tool_action(ActionID::CLIPPING_PLANE_WINDOW, "clipping_planes");
    add_tool_action(ActionID::SELECTION_FILTER, "selection_filter");

    m_view_options_menu->append("Selection filter", "win.selection_filter");
    m_view_options_menu->append("Clipping planes", "win.clipping_planes");
    m_view_options_menu->append("Previous construction entities", "win.previous_construction");
    m_view_options_menu->append("Perspective projection", "win.perspective");
    {
        auto it = Gio::MenuItem::create("scale", "scale");
        it->set_attribute_value("custom", Glib::Variant<Glib::ustring>::create("scale"));

        m_view_options_menu->append_item(it);
    }

    view_options_popover->set_menu_model(m_view_options_menu);

    {
        auto adj = Gtk::Adjustment::create(-1, -1, 5, .01, .1);
        m_curvature_comb_scale = Gtk::make_managed<Gtk::Scale>(adj);
        m_curvature_comb_scale->add_mark(adj->get_lower(), Gtk::PositionType::BOTTOM, "Off");

        adj->signal_value_changed().connect([this, adj] {
            if (!m_core.has_documents())
                return;
            float scale = 0;
            auto val = adj->get_value();
            if (val > adj->get_lower())
                scale = powf(10, val);
            auto &wv = m_workspace_views.at(m_current_workspace_view);
            wv.m_curvature_comb_scale = scale;
            canvas_update_keep_selection();
            update_view_hints();
        });

        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
        box->set_margin_start(32);
        box->set_margin_top(6);
        auto label = Gtk::make_managed<Gtk::Label>("Curvature comb scale");
        label->set_xalign(0);
        box->append(*label);
        box->append(*m_curvature_comb_scale);

        view_options_popover->add_child(*box, "scale");
    }
}

Gtk::Button &Editor::create_action_bar_button(ActionToolID action)
{
    static const std::map<ActionToolID, std::string> action_icons = {
            {ToolID::DRAW_CONTOUR, "action-draw-contour-symbolic"},
            {ToolID::DRAW_CIRCLE_2D, "action-draw-line-circle-symbolic"},
            {ToolID::DRAW_RECTANGLE, "action-draw-line-rectangle-symbolic"},
            {ToolID::DRAW_REGULAR_POLYGON, "action-draw-line-regular-polygon-symbolic"},
            {ToolID::DRAW_WORKPLANE, "action-draw-workplane-symbolic"},
            {ToolID::DRAW_TEXT, "action-draw-text-symbolic"},
    };
    auto bu = Gtk::make_managed<Gtk::Button>();
    bu->set_tooltip_text(action_catalog.at(action).name);
    auto img = Gtk::make_managed<Gtk::Image>();
    if (action_icons.count(action))
        img->set_from_icon_name(action_icons.at(action));
    else
        img->set_from_icon_name("face-worried-symbolic");
    img->set_icon_size(Gtk::IconSize::LARGE);
    bu->set_child(*img);
    bu->add_css_class("osd");
    bu->add_css_class("action-button");
    bu->signal_clicked().connect([this, action] {
        if (force_end_tool())
            trigger_action(action);
    });
    m_win.add_action_button(*bu);
    m_action_bar_buttons.emplace(action, bu);
    return *bu;
}

void Editor::update_action_bar_buttons_sensitivity()
{
    auto has_docs = m_core.has_documents();
    for (auto &[act, bu] : m_action_bar_buttons) {
        auto sensitive = false;
        if (has_docs) {
            if (std::holds_alternative<ActionID>(act)) {
                auto a = std::get<ActionID>(act);
                if (m_action_sensitivity.contains(a))
                    sensitive = m_action_sensitivity.at(a);
            }
            else {
                sensitive = m_core.tool_can_begin(std::get<ToolID>(act), {}).get_can_begin();
            }
        }
        bu->set_sensitive(sensitive);
    }
}

void Editor::update_action_bar_visibility()
{
    bool visible = false;
    if (m_preferences.action_bar.enable && m_core.has_documents()) {
        auto tool_is_active = m_core.tool_is_active();
        if (m_preferences.action_bar.show_in_tool)
            visible = true;
        else
            visible = !tool_is_active;
    }
    m_win.set_action_bar_visible(visible);
}

static std::string action_tool_id_to_string(ActionToolID id)
{
    if (auto tool = std::get_if<ToolID>(&id))
        return "T_" + tool_lut.lookup_reverse(*tool);
    else if (auto act = std::get_if<ActionID>(&id))
        return "A_" + action_lut.lookup_reverse(*act);
    throw std::runtime_error("invalid action");
}

void Editor::init_canvas()
{
    {
        auto controller = Gtk::EventControllerKey::create();
        controller->signal_key_pressed().connect(
                [this, controller](guint keyval, guint keycode, Gdk::ModifierType state) -> bool {
                    return handle_action_key(controller, keyval, state);
                },
                true);

        get_canvas().add_controller(controller);
    }
    {
        auto controller = Gtk::GestureClick::create();
        controller->set_button(0);
        controller->signal_pressed().connect([this, controller](int n_press, double x, double y) {
            auto button = controller->get_current_button();
            if (n_press == 2 && button == 2) {
                trigger_action(ActionID::VIEW_RESET_TILT);
                return;
            }
            if (button == 3) {
                m_rmb_last_x = x;
                m_rmb_last_y = y;
            }
            if (button == 1 /*|| button == 3*/)
                handle_click(button, n_press);
            else if (button == 8)
                trigger_action(ActionID::NEXT_GROUP);
            else if (button == 9)
                trigger_action(ActionID::PREVIOUS_GROUP);
        });

        controller->signal_released().connect([this, controller](int n_press, double x, double y) {
            m_drag_tool = ToolID::NONE;
            if (controller->get_current_button() == 1 && n_press == 1) {
                if (m_core.tool_is_active()) {
                    ToolArgs args;
                    args.type = ToolEventType::ACTION;
                    args.action = InToolActionID::LMB_RELEASE;
                    ToolResponse r = m_core.tool_update(args);
                    tool_process(r);
                }
            }
            else if (controller->get_current_button() == 3 && n_press == 1) {
                const auto dist = glm::length(glm::vec2(x, y) - glm::vec2(m_rmb_last_x, m_rmb_last_y));
                if (dist > 16)
                    return;
                if (m_core.tool_is_active())
                    handle_click(3, 1);
                else
                    open_context_menu();
            }
        });

        get_canvas().add_controller(controller);
    }
    {
        auto controller = Gtk::EventControllerMotion::create();
        controller->signal_motion().connect([this](double x, double y) {
            if (m_last_x != x || m_last_y != y) {
                m_last_x = x;
                m_last_y = y;
            }
        });
        get_canvas().add_controller(controller);
    }
    get_canvas().signal_cursor_moved().connect(sigc::mem_fun(*this, &Editor::handle_cursor_move));

    m_context_menu = Gtk::make_managed<Gtk::PopoverMenu>();
#if GTK_CHECK_VERSION(4, 14, 0)
    gtk_popover_menu_set_flags(m_context_menu->gobj(), GTK_POPOVER_MENU_NESTED);
#endif
    m_context_menu->set_parent(get_canvas());

    auto actions = Gio::SimpleActionGroup::create();
    for (const auto &[id, act] : action_catalog) {
        std::string name;
        auto action_id = id;
        actions->add_action(action_tool_id_to_string(id), [this, action_id] {
            get_canvas().set_selection(m_context_menu_selection, false);
            trigger_action(action_id);
        });
    }

    actions->add_action_with_parameter(
            "remove_constraint", Glib::Variant<std::string>::variant_type(), [this](Glib::VariantBase const &value) {
                UUID uu = Glib::VariantBase::cast_dynamic<Glib::Variant<std::string>>(value).get();
                auto &doc = m_core.get_current_document();
                doc.m_constraints.erase(uu);
                doc.set_group_solve_pending(m_core.get_current_group());
                m_core.set_needs_save();
                m_core.rebuild("remove constraint");
                canvas_update_keep_selection();
            });
    m_context_menu->insert_action_group("menu", actions);


    get_canvas().signal_hover_selection_changed().connect([this] {
        auto hsel = get_canvas().get_hover_selection();
        if (!hsel) {
            get_canvas().set_highlight({});
            return;
        }
        if (hsel->type != SelectableRef::Type::CONSTRAINT) {
            get_canvas().set_highlight({});
            return;
        }
        auto &constraint = m_core.get_current_document().get_constraint(hsel->item);
        std::set<SelectableRef> sel;
        std::map<UUID, std::set<unsigned int>> enps;
        UUID constraint_wrkpl;
        if (auto co_wrkpl = dynamic_cast<const IConstraintWorkplane *>(&constraint))
            constraint_wrkpl = co_wrkpl->get_workplane(m_core.get_current_document());
        for (const auto &enp : constraint.get_referenced_entities_and_points()) {
            // ignore constraint workplanes
            if (enp.entity == constraint_wrkpl)
                continue;
            sel.emplace(SelectableRef::Type::ENTITY, enp.entity, enp.point);
            if (enp.point != 0)
                sel.emplace(SelectableRef::Type::ENTITY, enp.entity, 0);
            enps[enp.entity].insert(enp.point);
        }
        for (const auto &[uu, pts] : enps) {
            auto &entity = m_core.get_current_document().get_entity(uu);
            if (entity.of_type(Entity::Type::LINE_2D, Entity::Type::LINE_3D)) {
                if (pts.contains(1) && pts.contains(2)) {
                    sel.emplace(SelectableRef::Type::ENTITY, uu, 0);
                }
            }
        }
        get_canvas().set_highlight(sel);
    });

    get_canvas().signal_selection_mode_changed().connect(sigc::mem_fun(*this, &Editor::update_selection_mode_label));
    update_selection_mode_label();

    get_canvas().signal_query_tooltip().connect(
            [this](int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip> &tooltip) {
                if (keyboard_tooltip)
                    return false;
                auto sr = get_canvas().get_hover_selection();
                if (!sr.has_value())
                    return false;

                auto tip = get_selectable_ref_description(m_core, m_core.get_current_idocument_info().get_uuid(), *sr);
                tooltip->set_text(tip);

                return true;
            },
            true);
    get_canvas().set_has_tooltip(true);

    get_canvas().set_selection_menu_creator(m_selection_menu_creator);

    /*
     * we want the canvas click event controllers to run before the one in the editor,
     * so we need to attach them afterwards since event controllers attached last run first
     */
    get_canvas().setup_controllers();
}

void Editor::update_selection_mode_label()
{
    const auto mode = get_canvas().get_selection_mode();
    std::string label;
    switch (mode) {
    case SelectionMode::HOVER:
        m_win.set_selection_mode_label_text("Hover select");
        break;
    case SelectionMode::NORMAL:
        m_win.set_selection_mode_label_text("Click select");
        break;
    default:;
    }
}

namespace {
class ContextMenuButton : public Gtk::Button {
public:
    typedef sigc::signal<void(bool)> type_signal_show_preview;
    type_signal_show_preview signal_show_preview()
    {
        return m_signal_show_preview;
    }

    ContextMenuButton()
    {
        auto ctrl = Gtk::EventControllerMotion::create();
        ctrl->signal_leave().connect([this] {
            if (m_preview)
                m_signal_show_preview.emit(false);
            m_preview = false;
            m_timeout_connection.disconnect();
        });
        ctrl->signal_motion().connect([this](double, double) {
            m_timeout_connection.disconnect();
            m_timeout_connection = Glib::signal_timeout().connect(
                    [this] {
                        if (!m_preview)
                            m_signal_show_preview.emit(true);
                        m_preview = true;
                        return false;
                    },
                    200);
        });
        add_controller(ctrl);
    };

private:
    sigc::connection m_timeout_connection;
    type_signal_show_preview m_signal_show_preview;
    bool m_preview = false;
};
} // namespace

void Editor::open_context_menu()
{
    Gdk::Rectangle rect;
    rect.set_x(m_last_x);
    rect.set_y(m_last_y);

    m_context_menu->set_pointing_to(rect);
    get_canvas().end_pan();
    auto menu = Gio::Menu::create();
    auto sel = get_canvas().get_selection();
    auto hover_sel = get_canvas().get_hover_selection();
    if (!hover_sel)
        return;
    if (!sel.contains(*hover_sel))
        sel = {*hover_sel};
    m_context_menu_selection = sel;
    update_action_sensitivity(sel);
    struct ActionInfo {
        ActionToolID id;
        bool can_preview;
    };
    std::vector<ActionInfo> ids;

    std::list<Glib::RefPtr<Gio::MenuItem>> meas_items;
    for (const auto &[action_group, action_group_name] : action_group_catalog) {
        for (const auto &[id, it_cat] : action_catalog) {
            if (it_cat.group == action_group && !(it_cat.flags & ActionCatalogItem::FLAGS_NO_MENU)) {
                if (auto tool = std::get_if<ToolID>(&id)) {
                    auto r = m_core.tool_can_begin(*tool, sel);
                    if (r.can_begin == ToolBase::CanBegin::YES && r.is_specific) {
                        ids.emplace_back(id, r.can_preview);
                        auto item = Gio::MenuItem::create(it_cat.name, "menu." + action_tool_id_to_string(id));
                        if (it_cat.group == ActionGroup::MEASURE) {
                            meas_items.push_back(item);
                        }
                        else {
                            item->set_attribute_value(
                                    "custom", Glib::Variant<Glib::ustring>::create(action_tool_id_to_string(id)));
                            menu->append_item(item);
                        }
                    }
                }
                else if (auto act = std::get_if<ActionID>(&id)) {
                    if (get_action_sensitive(*act) && (it_cat.flags & ActionCatalogItem::FLAGS_SPECIFIC)) {
                        ids.emplace_back(id, false);
                        auto item = Gio::MenuItem::create(it_cat.name, "menu." + action_tool_id_to_string(id));
                        item->set_attribute_value("custom",
                                                  Glib::Variant<Glib::ustring>::create(action_tool_id_to_string(id)));
                        menu->append_item(item);
                    }
                }
            }
        }
    }
    if (meas_items.size() > 1) {
        auto measurement_submenu = Gio::Menu::create();
        for (auto it : meas_items) {
            measurement_submenu->append_item(it);
        }
        auto item = Gio::MenuItem::create("Measure", measurement_submenu);
        menu->append_item(item);
    }
    else if (meas_items.size() == 1) {
        menu->append_item(meas_items.front());
    }

    if (m_core.has_documents()) {
        auto &doc = m_core.get_current_document();
        if (auto enp = point_from_selection(doc, m_context_menu_selection)) {
            auto &en = doc.get_entity(enp->entity);
            std::vector<const Constraint *> constraints;
            for (auto constraint : en.get_constraints(doc)) {
                if (enp->point == 0 || constraint->get_referenced_entities_and_points().contains(*enp))
                    constraints.push_back(constraint);
            }
            std::ranges::sort(constraints, [](auto a, auto b) { return a->get_type() < b->get_type(); });

            if (constraints.size()) {
                auto submenu = Gio::Menu::create();
                for (auto constraint : constraints) {
                    auto item = Gio::MenuItem::create(constraint->get_type_name(), "menu.remove_constraint");
                    item->set_action_and_target("menu.remove_constraint",
                                                Glib::Variant<std::string>::create(constraint->m_uuid));
                    submenu->append_item(item);
                }
                auto item = Gio::MenuItem::create("Remove constraint", submenu);
                menu->append_item(item);
            }
        }
    }
    if (menu->get_n_items() != 0) {
        m_context_menu->set_menu_model(menu);
        for (const auto [id, can_preview] : ids) {
            auto button = Gtk::make_managed<ContextMenuButton>();
            button->signal_clicked().connect([this, id] {
                m_context_menu->popdown();
                get_canvas().set_selection(m_context_menu_selection, false);
                trigger_action(id);
            });
            if (m_preferences.editor.preview_constraints && can_preview) {
                button->signal_show_preview().connect([this, id](bool preview) {
                    m_context_menu->set_opacity(preview ? .25 : 1);
                    if (preview) {
                        m_core.apply_preview(std::get<ToolID>(id), m_context_menu_selection);
                        canvas_update_keep_selection();
                    }
                    else {
                        m_core.reset_preview();
                        canvas_update_keep_selection();
                    }
                });
            }
            button->add_css_class("context-menu-button");
            auto label = Gtk::make_managed<Gtk::Label>(action_catalog.at(id).name);
            label->set_xalign(0);
            label->set_hexpand(true);
            auto label2 =
                    Gtk::make_managed<Gtk::Label>(key_sequences_to_string(m_action_connections.at(id).key_sequences));
            label2->add_css_class("dim-label");
            auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
            box->append(*label);
            box->append(*label2);
            button->set_child(*box);

            button->set_has_frame(false);
            m_context_menu->add_child(*button, action_tool_id_to_string(id));
        }
        m_context_menu->popup();
    }
}

void Editor::init_properties_notebook()
{
    m_properties_notebook = Gtk::make_managed<Gtk::Notebook>();
    m_properties_notebook->set_show_border(false);
    m_properties_notebook->set_tab_pos(Gtk::PositionType::BOTTOM);
    m_win.get_left_bar().set_end_child(*m_properties_notebook);
    {
        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
        m_group_editor_box = Gtk::make_managed<Gtk::Box>();
        m_group_editor_box->set_vexpand(true);
        box->append(*m_group_editor_box);
        auto label = Gtk::make_managed<Gtk::Label>("Commit pending");
        label->set_margin(3);
        m_group_commit_pending_revealer = Gtk::make_managed<Gtk::Revealer>();
        m_group_commit_pending_revealer->set_transition_type(Gtk::RevealerTransitionType::CROSSFADE);
        m_group_commit_pending_revealer->set_child(*label);
        box->append(*m_group_commit_pending_revealer);
        m_properties_notebook->append_page(*box, "Group");
    }
    m_core.signal_rebuilt().connect([this] {
        if (m_group_editor) {
            if (m_core.get_current_group() != m_group_editor->get_group())
                update_group_editor();
            else
                m_group_editor->reload();
        }
    });


    m_constraints_box = Gtk::make_managed<ConstraintsBox>(m_core);
    m_properties_notebook->append_page(*m_constraints_box, "Constraints");
    m_core.signal_rebuilt().connect([this] { m_constraints_box->update(); });
    m_core.signal_documents_changed().connect([this] { m_constraints_box->update(); });
    m_constraints_box->signal_constraint_selected().connect([this](const UUID &uu) {
        SelectableRef sr{.type = SelectableRef::Type::CONSTRAINT, .item = uu};
        get_canvas().set_selection({sr}, true);
        get_canvas().set_selection_mode(SelectionMode::NORMAL);
    });
    m_constraints_box->signal_changed().connect([this] { canvas_update_keep_selection(); });

    {
        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);

        m_selection_editor = Gtk::make_managed<SelectionEditor>(m_core, static_cast<IDocumentViewProvider &>(*this));
        m_selection_editor->signal_changed().connect(sigc::mem_fun(*this, &Editor::handle_commit_from_editor));
        m_selection_editor->signal_view_changed().connect([this] { canvas_update_keep_selection(); });
        m_selection_editor->set_vexpand(true);
        box->append(*m_selection_editor);
        auto label = Gtk::make_managed<Gtk::Label>("Commit pending");
        label->set_margin(3);
        m_selection_commit_pending_revealer = Gtk::make_managed<Gtk::Revealer>();
        m_selection_commit_pending_revealer->set_transition_type(Gtk::RevealerTransitionType::CROSSFADE);
        m_selection_commit_pending_revealer->set_child(*label);
        box->append(*m_selection_commit_pending_revealer);
        m_properties_notebook->append_page(*box, "Selection");
    }
    get_canvas().signal_selection_changed().connect(sigc::mem_fun(*this, &Editor::update_selection_editor));
}

void Editor::update_selection_editor()
{
    if (get_canvas().get_selection_mode() == SelectionMode::HOVER)
        m_selection_editor->set_selection({});
    else
        m_selection_editor->set_selection(get_canvas().get_selection());
}

void Editor::init_header_bar()
{
    attach_action_button(m_win.get_open_button(), ActionID::OPEN_DOCUMENT);
    attach_action_sensitive(m_win.get_open_menu_button(), ActionID::OPEN_DOCUMENT);
    attach_action_button(m_win.get_new_button(), ActionID::NEW_DOCUMENT);
    attach_action_button(m_win.get_save_button(), ActionID::SAVE);
    attach_action_button(m_win.get_save_as_button(), ActionID::SAVE_AS);

    {
        auto undo_redo_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 0));
        undo_redo_box->add_css_class("linked");

        auto undo_button = create_action_button(ActionID::UNDO);
        undo_button->set_tooltip_text("Undo");
        undo_button->set_image_from_icon_name("edit-undo-symbolic");
        undo_redo_box->append(*undo_button);

        auto redo_button = create_action_button(ActionID::REDO);
        redo_button->set_tooltip_text("Redo");
        redo_button->set_image_from_icon_name("edit-redo-symbolic");
        undo_redo_box->append(*redo_button);

        m_win.get_header_bar().pack_start(*undo_redo_box);
    }

    {
        auto top = Gio::Menu::create();

        top->append_item(Gio::MenuItem::create("Preferences", "app.preferences"));
        top->append_item(Gio::MenuItem::create("Logger", "app.logger"));
        top->append_item(Gio::MenuItem::create("About", "app.about"));

        m_win.get_hamburger_menu_button().set_menu_model(top);
    }
}

void Editor::update_view_hints()
{
    std::vector<std::string> hints;
    if (get_canvas().get_projection() == Canvas::Projection::PERSP)
        hints.push_back("persp.");
    {
        const auto cl = get_canvas().get_clipping_planes();
        if (cl.x.enabled || cl.y.enabled || cl.z.enabled) {
            std::string s = "clipped:";
            if (cl.x.enabled)
                s += "x";
            if (cl.y.enabled)
                s += "y";
            if (cl.z.enabled)
                s += "z";
            hints.push_back(s);
        }
    }
    if (m_selection_filter_window->is_active())
        hints.push_back("selection filtered");
    if (m_core.has_documents()) {
        if (get_current_document_view().construction_entities_from_previous_groups_are_visible())
            hints.push_back("prev. construction entities");
        auto &wv = m_workspace_views.at(m_current_workspace_view);
        if (wv.m_curvature_comb_scale > 0)
            hints.push_back("curv. combs");
    }
    m_win.set_view_hints_label(hints);
}

void Editor::on_open_document(const ActionConnection &conn)
{
    auto dialog = Gtk::FileDialog::create();
    if (m_core.has_documents()) {
        if (m_core.get_current_idocument_info().has_path()) {
            dialog->set_initial_file(
                    Gio::File::create_for_path(path_to_string(m_core.get_current_idocument_info().get_path())));
        }
    }

    // Add filters, so that only certain file types can be selected:
    auto filters = Gio::ListStore<Gtk::FileFilter>::create();

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("Dune 3D documents");
    filter_any->add_pattern("*.d3ddoc");
    filters->append(filter_any);

    dialog->set_filters(filters);

    // Show the dialog and wait for a user response:
    dialog->open(m_win, [this, dialog](const Glib::RefPtr<Gio::AsyncResult> &result) {
        try {
            auto file = dialog->open_finish(result);
            m_win.open_file_view(file);
            // Notice that this is a std::string, not a Glib::ustring.
            auto filename = file->get_path();
            std::cout << "File selected: " << filename << std::endl;
        }
        catch (const Gtk::DialogError &err) {
            // Can be thrown by dialog->open_finish(result).
            std::cout << "No file selected. " << err.what() << std::endl;
        }
        catch (const Glib::Error &err) {
            std::cout << "Unexpected exception. " << err.what() << std::endl;
        }
    });
}


void Editor::on_save_as(const ActionConnection &conn)
{
    auto dialog = Gtk::FileDialog::create();
    if (m_core.get_current_idocument_info().has_path()) {
        dialog->set_initial_file(
                Gio::File::create_for_path(path_to_string(m_core.get_current_idocument_info().get_path())));
    }

    // Add filters, so that only certain file types can be selected:
    auto filters = Gio::ListStore<Gtk::FileFilter>::create();

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("Dune 3D documents");
    filter_any->add_pattern("*.d3ddoc");
    filters->append(filter_any);

    dialog->set_filters(filters);

    // Show the dialog and wait for a user response:
    dialog->save(m_win, [this, dialog](const Glib::RefPtr<Gio::AsyncResult> &result) {
        try {
            auto file = dialog->save_finish(result);
            // open_file_view(file);
            //  Notice that this is a std::string, not a Glib::ustring.
            auto filename = path_from_string(append_suffix_if_required(file->get_path(), ".d3ddoc"));
            // std::cout << "File selected: " << filename << std::endl;
            m_win.get_app().add_recent_item(filename);
            m_core.save_as(filename);
            save_workspace_view(m_core.get_current_idocument_info().get_uuid());
            m_workspace_browser->update_documents(get_current_document_views());
            update_version_info();
            update_title();
            if (m_after_save_cb)
                m_after_save_cb();
        }
        catch (const Gtk::DialogError &err) {
            // Can be thrown by dialog->open_finish(result).
            std::cout << "No file selected. " << err.what() << std::endl;
        }
        catch (const Glib::Error &err) {
            std::cout << "Unexpected exception. " << err.what() << std::endl;
        }
    });
}


void Editor::init_tool_popover()
{
    m_tool_popover = Gtk::make_managed<ToolPopover>();
    m_tool_popover->set_parent(get_canvas());
    m_tool_popover->signal_action_activated().connect([this](ActionToolID action_id) { trigger_action(action_id); });


    connect_action({ActionID::POPOVER}, [this](const auto &a) {
        Gdk::Rectangle rect;
        rect.set_x(m_last_x);
        rect.set_y(m_last_y);

        m_tool_popover->set_pointing_to(rect);

        this->update_action_sensitivity();
        std::map<ActionToolID, bool> can_begin;
        auto sel = get_canvas().get_selection();
        for (const auto &[id, it] : action_catalog) {
            if (std::holds_alternative<ToolID>(id)) {
                bool r = m_core.tool_can_begin(std::get<ToolID>(id), sel).get_can_begin();
                can_begin[id] = r;
            }
            else {
                can_begin[id] = this->get_action_sensitive(std::get<ActionID>(id));
            }
        }
        m_tool_popover->set_can_begin(can_begin);

        m_tool_popover->popup();
    });
}

Canvas &Editor::get_canvas()
{
    return m_win.get_canvas();
}

const Canvas &Editor::get_canvas() const
{
    return m_win.get_canvas();
}


void Editor::show_save_dialog(const std::string &doc_name, std::function<void()> save_cb,
                              std::function<void()> no_save_cb)
{
    auto dialog = Gtk::AlertDialog::create("Save changes to document \"" + doc_name + "\" before closing?");
    dialog->set_detail(
            "If you don't save, all your changes will be permanently "
            "lost.");
    dialog->set_buttons({"Cancel", "Close without saving", "Save"});
    dialog->set_cancel_button(0);
    dialog->set_default_button(0);
    dialog->choose(m_win, [dialog, save_cb, no_save_cb](Glib::RefPtr<Gio::AsyncResult> &result) {
        auto btn = dialog->choose_finish(result);
        if (btn == 1) {
            if (no_save_cb)
                no_save_cb();
        }
        else if (btn == 2)
            if (save_cb)
                save_cb();
    });
}

void Editor::close_document(const UUID &doc_uu, std::function<void()> save_cb, std::function<void()> no_save_cb)
{
    const auto &doci = m_core.get_idocument_info(doc_uu);
    if (doci.get_needs_save()) {
        show_save_dialog(
                doci.get_basename(),
                [this, doc_uu, save_cb, no_save_cb] {
                    m_after_save_cb = [this, doc_uu, save_cb] {
                        m_core.close_document(doc_uu);
                        auto_close_workspace_views();
                        if (save_cb)
                            save_cb();
                    };
                    trigger_action(ActionID::SAVE);
                },
                [this, doc_uu, no_save_cb] {
                    m_core.close_document(doc_uu);
                    auto_close_workspace_views();
                    if (no_save_cb)
                        no_save_cb();
                });
    }
    else {
        m_core.close_document(doc_uu);
        auto_close_workspace_views();
    }
}

void Editor::update_group_editor()
{
    if (m_group_editor) {
        if (m_delayed_commit_connection.connected()) {
            commit_from_editor();
        }
        m_group_editor_box->remove(*m_group_editor);
        m_group_editor = nullptr;
    }
    if (!m_core.has_documents())
        return;
    m_group_editor = GroupEditor::create(m_core, m_core.get_current_group());
    m_group_editor->signal_changed().connect(sigc::mem_fun(*this, &Editor::handle_commit_from_editor));
    m_group_editor->signal_trigger_action().connect([this](auto act) { trigger_action(act); });
    m_group_editor_box->append(*m_group_editor);
}

void Editor::handle_commit_from_editor(CommitMode mode)
{
    if (mode == CommitMode::DELAYED) {
        m_core.get_current_document().update_pending(m_core.get_current_group());
        m_delayed_commit_connection.disconnect(); // stop old timer
        m_delayed_commit_connection = Glib::signal_timeout().connect(
                [this] {
                    commit_from_editor();
                    return false;
                },
                1000);
        m_group_commit_pending_revealer->set_reveal_child(true);
        m_selection_commit_pending_revealer->set_reveal_child(true);
    }
    else if (mode == CommitMode::IMMEDIATE
             || (mode == CommitMode::EXECUTE_DELAYED && m_delayed_commit_connection.connected())) {
        commit_from_editor();
    }
    m_core.set_needs_save();
    canvas_update_keep_selection();
}

void Editor::commit_from_editor()
{
    m_delayed_commit_connection.disconnect();
    m_group_commit_pending_revealer->set_reveal_child(false);
    m_selection_commit_pending_revealer->set_reveal_child(false);
    m_core.rebuild("group/selection edited");
}

void Editor::update_workplane_label()
{
    if (!m_core.has_documents()) {
        m_win.set_workplane_label("No documents");
        return;
    }
    auto wrkpl_uu = m_core.get_current_workplane();
    if (!wrkpl_uu) {
        m_win.set_workplane_label("No Workplane");
    }
    else {
        auto &wrkpl = m_core.get_current_document().get_entity<EntityWorkplane>(wrkpl_uu);
        auto &wrkpl_group = m_core.get_current_document().get_group<Group>(wrkpl.m_group);
        std::string s = "Workplane ";
        if (wrkpl.m_name.size())
            s += wrkpl.m_name + " ";
        s += "in group " + wrkpl_group.m_name;
        m_win.set_workplane_label(s);
    }
}

KeyMatchResult Editor::keys_match(const KeySequence &keys) const
{
    return key_sequence_match(m_keys_current, keys);
}

void Editor::apply_preferences()
{
    for (auto &[id, conn] : m_action_connections) {
        auto &act = action_catalog.at(id);
        if (!(act.flags & ActionCatalogItem::FLAGS_NO_PREFERENCES) && m_preferences.key_sequences.keys.count(id)) {
            conn.key_sequences = m_preferences.key_sequences.keys.at(id);
        }
    }
    m_in_tool_key_sequeces_preferences = m_preferences.in_tool_key_sequences;
    m_in_tool_key_sequeces_preferences.keys.erase(InToolActionID::LMB);
    m_in_tool_key_sequeces_preferences.keys.erase(InToolActionID::RMB);
    m_in_tool_key_sequeces_preferences.keys.erase(InToolActionID::LMB_RELEASE);

    {
        const auto mod0 = static_cast<Gdk::ModifierType>(0);

        m_in_tool_key_sequeces_preferences.keys[InToolActionID::CANCEL] = {{{GDK_KEY_Escape, mod0}}};
        m_in_tool_key_sequeces_preferences.keys[InToolActionID::COMMIT] = {{{GDK_KEY_Return, mod0}},
                                                                           {{GDK_KEY_KP_Enter, mod0}}};
    }

    for (const auto &[id, it] : m_action_connections) {
        if (it.key_sequences.size()) {
            m_tool_popover->set_key_sequences(id, it.key_sequences);
        }
    }

    auto dark = Gtk::Settings::get_default()->property_gtk_application_prefer_dark_theme().get_value();
    if (dark != m_preferences.canvas.dark_theme)
        Gtk::Settings::get_default()->property_gtk_application_prefer_dark_theme().set_value(
                m_preferences.canvas.dark_theme);
    dark = Gtk::Settings::get_default()->property_gtk_application_prefer_dark_theme().get_value();
    switch (m_preferences.canvas.theme_variant) {
    case CanvasPreferences::ThemeVariant::AUTO:
        break;
    case CanvasPreferences::ThemeVariant::DARK:
        dark = true;
        break;
    case CanvasPreferences::ThemeVariant::LIGHT:
        dark = false;
        break;
    }
    if (color_themes.contains(m_preferences.canvas.theme)) {
        Appearance appearance = m_preferences.canvas.appearance;
        appearance.colors = color_themes.at(m_preferences.canvas.theme).get(dark);
        get_canvas().set_appearance(appearance);
    }
    else {
        get_canvas().set_appearance(m_preferences.canvas.appearance);
    }
    get_canvas().set_enable_animations(m_preferences.canvas.enable_animations);
    get_canvas().set_zoom_to_cursor(m_preferences.canvas.zoom_to_cursor);
    get_canvas().set_rotation_scheme(m_preferences.canvas.rotation_scheme);

    m_win.tool_bar_set_vertical(m_preferences.tool_bar.vertical_layout);
    update_action_bar_visibility();
    update_error_overlay();
    canvas_update_keep_selection();
    /*
        key_sequence_dialog->clear();
        for (const auto &it : action_connections) {
            if (it.second.key_sequences.size()) {
                key_sequence_dialog->add_sequence(it.second.key_sequences, action_catalog.at(it.first).name);
                tool_popover->set_key_sequences(it.first, it.second.key_sequences);
            }
        }
        preferences_apply_to_canvas(canvas, preferences);
        for (auto it : action_buttons) {
            it->update_key_sequences();
            it->set_keep_primary_action(!preferences.action_bar.remember);
        }
        main_window->set_use_action_bar(preferences.action_bar.enable);
        m_core.set_history_max(preferences.undo_redo.max_depth);
        m_core.set_history_never_forgets(preferences.undo_redo.never_forgets);
        selection_history_manager.set_never_forgets(preferences.undo_redo.never_forgets);
        preferences_apply_appearance(preferences);
        */
}

void Editor::update_error_overlay()
{
    if (m_core.has_documents()) {
        auto &doc = m_core.get_current_document();
        auto &group = doc.get_group(m_core.get_current_group());
        get_canvas().set_show_error_overlay(m_preferences.canvas.error_overlay
                                            && group.m_solve_result != SolveResult::OKAY);
    }
    else {
        get_canvas().set_show_error_overlay(false);
    }
}

void Editor::render_document(const IDocumentInfo &doc)
{
    auto &doc_view = get_current_document_views()[doc.get_uuid()];
    if (!doc_view.m_document_is_visible && (doc.get_uuid() != m_core.get_current_idocument_info().get_uuid()))
        return;
    std::optional<SelectableRef> sr;
    if (doc.get_uuid() != m_core.get_current_idocument_info().get_uuid()) {
        sr = SelectableRef{SelectableRef::Type::DOCUMENT, doc.get_uuid()};
    }
    Renderer renderer(get_canvas(), m_core);
    renderer.m_solid_model_edge_select_mode = m_solid_model_edge_select_mode;
    renderer.m_curvature_comb_scale = m_workspace_views.at(m_current_workspace_view).m_curvature_comb_scale;
    renderer.m_connect_curvature_comb = m_preferences.canvas.connect_curvature_combs;

    if (doc.get_uuid() == m_core.get_current_idocument_info().get_uuid())
        renderer.add_constraint_icons(m_constraint_tip_pos, m_constraint_tip_vec, m_constraint_tip_icons);

    renderer.render(doc.get_document(), doc.get_current_group(), doc_view, doc.get_dirname(), sr);
}
void Editor::canvas_update()
{
    auto docs = m_core.get_documents();
    auto hover_sel = get_canvas().get_hover_selection();
    get_canvas().clear();

    if (m_core.has_documents())
        render_document(m_core.get_current_idocument_info());

    for (const auto doc : docs) {
        if (doc->get_uuid() != m_core.get_current_idocument_info().get_uuid())
            render_document(*doc);
    }

    get_canvas().set_hover_selection(hover_sel);
    update_error_overlay();
    get_canvas().request_push();
}

void Editor::canvas_update_keep_selection()
{
    auto sel = get_canvas().get_selection();
    canvas_update();
    get_canvas().set_selection(sel, false);
}

void Editor::enable_hover_selection()
{
    get_canvas().set_selection_mode(SelectionMode::HOVER_ONLY);
}

std::optional<SelectableRef> Editor::get_hover_selection() const
{
    return get_canvas().get_hover_selection();
}

glm::dvec3 Editor::get_cursor_pos() const
{
    return get_canvas().get_cursor_pos();
}

glm::vec3 Editor::get_cam_normal() const
{
    return get_canvas().get_cam_normal();
}

glm::dvec3 Editor::get_cursor_pos_for_plane(glm::dvec3 origin, glm::dvec3 normal) const
{
    return get_canvas().get_cursor_pos_for_plane(origin, normal);
}

void Editor::set_canvas_selection_mode(SelectionMode mode)
{
    m_last_selection_mode = mode;
}

void Editor::handle_cursor_move()
{
    if (m_core.tool_is_active()) {
        ToolArgs args;
        args.type = ToolEventType::MOVE;
        ToolResponse r = m_core.tool_update(args);
        tool_process(r);
    }
    else {
        if (m_drag_tool == ToolID::NONE)
            return;
        if (m_selection_for_drag.size() == 0)
            return;
        auto pos = get_canvas().get_cursor_pos_win();
        auto delta = pos - m_cursor_pos_win_drag_begin;
        if (glm::length(delta) > 10) {
            ToolArgs args;
            args.selection = m_selection_for_drag;
            m_last_selection_mode = get_canvas().get_selection_mode();
            get_canvas().set_selection_mode(SelectionMode::NONE);
            ToolResponse r = m_core.tool_begin(m_drag_tool, args, true);
            tool_process(r);

            m_selection_for_drag.clear();
            m_drag_tool = ToolID::NONE;
        }
    }
}

void Editor::handle_click(unsigned int button, unsigned int n)
{
    const bool is_doubleclick = n == 2;

    if (m_core.tool_is_active()) {
        const bool tool_supports_doubleclick = m_core.get_tool_actions().count(InToolActionID::LMB_DOUBLE);
        if (tool_supports_doubleclick || !is_doubleclick) {
            ToolArgs args;
            args.type = ToolEventType::ACTION;
            if (button == 1) {
                if (is_doubleclick)
                    args.action = InToolActionID::LMB_DOUBLE;
                else
                    args.action = InToolActionID::LMB;
            }
            else {
                args.action = InToolActionID::RMB;
            }
            ToolResponse r = m_core.tool_update(args);
            tool_process(r);
        }
    }
    else if (is_doubleclick && button == 1) {
        auto sel = get_canvas().get_hover_selection();
        if (sel) {
            if (auto action = get_doubleclick_action(*sel)) {
                get_canvas().set_selection({*sel}, false);
                trigger_action(*action);
            }
        }
    }
    else if (button == 1) {
        auto hover_sel = get_canvas().get_hover_selection();
        if (!hover_sel)
            return;

        auto sel = get_canvas().get_selection();
        if (sel.contains(hover_sel.value())) {
            m_drag_tool = get_tool_for_drag_move(false, sel);
            if (m_drag_tool != ToolID::NONE) {
                get_canvas().inhibit_drag_selection();
                m_cursor_pos_win_drag_begin = get_canvas().get_cursor_pos_win();
                // cursor_pos_grid_drag_begin = get_canvas().get_cursor_pos();
                m_selection_for_drag = sel;
            }
        }
    }
}

ToolID Editor::get_tool_for_drag_move(bool ctrl, const std::set<SelectableRef> &sel)
{
    return ToolID::MOVE;
}

void Editor::reset_key_hint_label()
{
    const auto act = ActionID::POPOVER;
    if (m_action_connections.count(act)) {
        if (m_action_connections.at(act).key_sequences.size()) {
            const auto keys = key_sequence_to_string(m_action_connections.at(act).key_sequences.front());
            m_win.set_key_hint_label_text("> " + keys + " for menu");
            return;
        }
    }
    m_win.set_key_hint_label_text(">");
}

void Editor::tool_bar_clear_actions()
{
    m_win.tool_bar_clear_actions();
    m_in_tool_action_label_infos.clear();
}

void Editor::tool_bar_set_actions(const std::vector<ActionLabelInfo> &labels)
{
    if (m_in_tool_action_label_infos != labels) {
        tool_bar_clear_actions();
        for (const auto &it : labels) {
            tool_bar_append_action(it.action1, it.action2, it.label);
        }

        m_in_tool_action_label_infos = labels;
    }
}

void Editor::tool_bar_append_action(InToolActionID action1, InToolActionID action2, const std::string &s)
{
    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 5);
    if (action1 == InToolActionID::LMB || action1 == InToolActionID::RMB) {
        std::string icon_name = "action-";
        if (action1 == InToolActionID::LMB) {
            icon_name += "lmb";
        }
        else {
            icon_name += "rmb";
        }
        icon_name += "-symbolic";
        auto img = Gtk::manage(new Gtk::Image);
        img->set_from_icon_name(icon_name);
        img->show();
        box->append(*img);
    }
    else {
        auto key_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
        for (const auto action : {action1, action2}) {
            if (action != InToolActionID::NONE) {
                const auto &prefs = m_in_tool_key_sequeces_preferences.keys;
                if (prefs.count(action)) {
                    if (prefs.at(action).size()) {
                        auto seq = prefs.at(action).front();
                        auto kl = Gtk::manage(new Gtk::Label(key_sequence_to_string_short(seq)));
                        kl->set_valign(Gtk::Align::BASELINE);
                        key_box->append(*kl);
                    }
                }
            }
        }
        key_box->get_style_context()->add_class("editor-key-box");

        box->append(*key_box);
    }
    const auto &as = s.size() ? s : in_tool_action_catalog.at(action1).name;

    auto la = Gtk::manage(new Gtk::Label(as));
    la->set_valign(Gtk::Align::BASELINE);

    la->show();

    box->append(*la);

    m_win.tool_bar_append_action(*box);
}

void Editor::update_version_info()
{
    if (!m_core.has_documents()) {
        m_win.set_version_info("");
        return;
    }
    const auto &doc = m_core.get_current_document();
    auto &ver = doc.m_version;
    m_win.set_version_info(ver.get_message());
}

bool Editor::has_file(const std::filesystem::path &path)
{
    return m_core.get_idocument_info_by_path(path);
}

void Editor::open_file(const std::filesystem::path &path)
{
    if (has_file(path))
        return;
    for (auto win : m_win.get_app().get_windows()) {
        if (auto appwin = dynamic_cast<Dune3DAppWindow *>(win)) {
            if (appwin->has_file(path)) {
                appwin->present();
                return;
            }
        }
    }
    add_to_recent_docs(path);
    try {
        const UUID doc_uu = UUID::random();

        std::map<UUID, WorkspaceView> loaded_workspace_views;
        try {
            const auto workspace_filename = get_workspace_filename_from_document_filename(path);
            if (std::filesystem::is_regular_file(workspace_filename)) {
                const auto j = load_json_from_file(workspace_filename);
                loaded_workspace_views = WorkspaceView::load_from_json(j.at("workspace_views"));
            }
        }
        catch (...) {
            loaded_workspace_views.clear();
        }

        DocumentView *new_dv = nullptr;
        UUID wsv;
        UUID current_wsv;
        if (loaded_workspace_views.size()) {
            for (const auto &[uu, wv] : loaded_workspace_views) {
                auto &dv = wv.m_documents.at({});
                auto r = m_workspace_views.emplace(uu, wv);
                auto &inserted_wv = r.first->second;
                inserted_wv.m_documents.emplace(doc_uu, dv);
                if (r.second) {
                    inserted_wv.m_current_document = doc_uu;
                    append_workspace_view_page(wv.m_name, uu);
                    set_current_workspace_view(uu);
                    current_wsv = uu;
                }
            }
        }
        else {
            wsv = create_workspace_view();
            m_workspace_views.at(wsv).m_current_document = doc_uu;
            set_current_workspace_view(wsv);
            auto &dv = m_workspace_views.at(wsv).m_documents[doc_uu];
            dv.m_document_is_visible = true;
            new_dv = &dv;
        }

        m_core.add_document(path, doc_uu);

        {
            auto &wv = m_workspace_views.at(m_current_workspace_view);
            m_core.set_current_document(wv.m_current_document);
            m_core.set_current_group(get_current_document_view().m_current_group);
        }

        if (new_dv) {
            new_dv->m_current_group = m_core.get_idocument_info(doc_uu).get_current_group();
        }
        if (current_wsv && m_core.get_current_idocument_info().get_uuid() == doc_uu) {
            auto &dv = m_workspace_views.at(current_wsv).m_documents[doc_uu];
            set_current_group(dv.m_current_group);
            set_show_previous_construction_entities(dv.m_show_construction_entities_from_previous_groups);
        }

        update_workspace_view_names();
        update_can_close_workspace_view_pages();
        m_win.get_app().add_recent_item(path);
        update_title();

        load_linked_documents(doc_uu);
    }
    CATCH_LOG(Logger::Level::WARNING, "error opening document" + path_to_string(path), Logger::Domain::DOCUMENT)
}

void Editor::load_linked_documents(const UUID &uu_doc)
{
    if (!m_core.has_documents())
        return;
    auto &doci = m_core.get_idocument_info(uu_doc);
    auto all_documents = m_core.get_documents();
    for (auto &[uu, en] : doci.get_document().m_entities) {
        if (auto en_doc = dynamic_cast<EntityDocument *>(en.get())) {
            // fill in referenced document
            const auto path = en_doc->get_path(doci.get_dirname());

            auto referenced_doc =
                    std::ranges::find_if(all_documents, [&path](const auto &x) { return x->get_path() == path; });
            if (referenced_doc == all_documents.end()) {
                open_file(path);
            }
        }
    }
}

void Editor::set_current_group(const UUID &uu_group)
{
    m_core.set_current_group(uu_group);
    m_workspace_browser->update_current_group(get_current_document_views());
    canvas_update_keep_selection();
    update_workplane_label();
    m_constraints_box->update();
    update_group_editor();
    update_action_sensitivity();
    update_action_bar_buttons_sensitivity();
    update_selection_editor();
}

void Editor::tool_bar_set_tool_tip(const std::string &s)
{
    m_win.tool_bar_set_tool_tip(s);
}

void Editor::tool_bar_flash(const std::string &s)
{
    m_win.tool_bar_flash(s);
}

void Editor::tool_bar_flash_replace(const std::string &s)
{
    m_win.tool_bar_flash_replace(s);
}

bool Editor::get_use_workplane() const
{
    return m_win.get_workplane_checkbutton().get_active();
}

void Editor::set_constraint_icons(glm::vec3 p, glm::vec3 v, const std::vector<ConstraintType> &constraints)
{
    m_constraint_tip_icons = constraints;
    m_constraint_tip_pos = p;
    m_constraint_tip_vec = v;
}

DocumentView &Editor::get_current_document_view()
{
    return m_workspace_views.at(m_current_workspace_view).m_documents[m_core.get_current_idocument_info().get_uuid()];
}

std::map<UUID, DocumentView> &Editor::get_current_document_views()
{
    return m_workspace_views.at(m_current_workspace_view).m_documents;
}

void Editor::update_title()
{
    if (m_core.has_documents()) {
        auto &doc = m_core.get_current_idocument_info();
        if (doc.has_path())
            m_win.set_window_title_from_path(doc.get_path());
        else
            m_win.set_window_title("New Document");
    }
    else {
        m_win.set_window_title("");
    }
}

Glib::RefPtr<Pango::Context> Editor::get_pango_context()
{
    return m_win.create_pango_context();
}


void Editor::set_buffer(std::unique_ptr<const Buffer> buffer)
{
    m_win.get_app().m_buffer = std::move(buffer);
}

const Buffer *Editor::get_buffer() const
{
    return m_win.get_app().m_buffer.get();
}

} // namespace dune3d

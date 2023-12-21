#include "editor.hpp"
#include "dune3d_appwindow.hpp"
#include "core/tool_id.hpp"
#include "widgets/constraints_box.hpp"
#include "action/action_id.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include "in_tool_action/in_tool_action_catalog.hpp"
#include "document/group/igroup_solid_model.hpp"
#include "document/solid_model.hpp"
#include "canvas/canvas.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/ientity_in_workplane.hpp"
#include "tool_popover.hpp"
#include "dune3d_application.hpp"
#include "preferences/preferences_window.hpp"
#include "document/group/all_groups.hpp"
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
#include "document/solid_model_util.hpp"
#include "document/export_paths.hpp"
#include "document/constraint/iconstraint_datum.hpp"
#include <iostream>

namespace dune3d {
Editor::Editor(Dune3DAppWindow &win, Preferences &prefs)
    : m_preferences(prefs), m_dialogs(win, *this), m_win(win), m_core(*this)
{
    m_drag_tool = ToolID::NONE;
}

void Editor::init()
{
    init_workspace_browser();
    init_properties_notebook();
    init_header_bar();
    init_actions();
    init_tool_popover();
    init_canvas();

    m_core.signal_needs_save().connect([this] { update_action_sensitivity(); });
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

    apply_preferences();

    m_core.signal_tool_changed().connect(sigc::mem_fun(*this, &Editor::handle_tool_change));


    m_core.signal_documents_changed().connect([this] {
        canvas_update_keep_selection();
        m_workspace_browser->update_documents(m_document_view);
        update_group_editor();
        update_workplane_label();
        update_action_sensitivity();
        m_workspace_browser->set_sensitive(m_core.has_documents());
        update_version_info();
    });


    update_action_sensitivity();
    reset_key_hint_label();
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

        m_win.add_controller(controller);
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
        for (const auto &uu : constraint.get_referenced_entities()) {
            // ignore workplanes
            auto &entity = m_core.get_current_document().get_entity(uu);
            if (entity.get_type() == Entity::Type::WORKPLANE)
                continue;
            sel.emplace(UUID(), SelectableRef::Type::ENTITY, uu, 0);
        }
        get_canvas().set_highlight(sel);
    });

    get_canvas().signal_selection_mode_changed().connect(sigc::mem_fun(*this, &Editor::update_selection_mode_label));
    update_selection_mode_label();

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
    for (const auto &[action_group, action_group_name] : action_group_catalog) {
        for (const auto &[id, it_cat] : action_catalog) {
            if (it_cat.group == action_group && !(it_cat.flags & ActionCatalogItem::FLAGS_NO_MENU)) {
                if (auto tool = std::get_if<ToolID>(&id)) {
                    auto r = m_core.tool_can_begin(*tool, sel);
                    if (r.can_begin && r.is_specific) {
                        auto item = Gio::MenuItem::create(it_cat.name, "menu." + action_tool_id_to_string(id));
                        menu->append_item(item);
                    }
                }
                else if (auto act = std::get_if<ActionID>(&id)) {
                    if (get_action_sensitive(*act) && (it_cat.flags & ActionCatalogItem::FLAGS_SPECIFIC)) {
                        auto item = Gio::MenuItem::create(it_cat.name, "menu." + action_tool_id_to_string(id));
                        menu->append_item(item);
                    }
                }
            }
        }
    }
    if (m_core.has_documents()) {
        auto &doc = m_core.get_current_document();
        if (auto en_uu = entity_from_selection(doc, m_context_menu_selection)) {
            auto &en = doc.get_entity(*en_uu);
            std::vector<const Constraint *> constraints;
            for (auto constraint : en.get_constraints(doc)) {
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
        m_group_editor_box = Gtk::make_managed<Gtk::Box>();
        m_properties_notebook->append_page(*m_group_editor_box, "Group");
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
        SelectableRef sr{.document = UUID(), .type = SelectableRef::Type::CONSTRAINT, .item = uu};
        get_canvas().set_selection({sr}, true);
        get_canvas().set_selection_mode(SelectionMode::NORMAL);
    });
    m_constraints_box->signal_changed().connect([this] { canvas_update_keep_selection(); });

    m_selection_editor = Gtk::make_managed<SelectionEditor>(m_core);
    m_selection_editor->signal_changed().connect([this] {
        m_core.set_needs_save();
        m_core.rebuild("selection edited");
        canvas_update_keep_selection();
    });
    m_properties_notebook->append_page(*m_selection_editor, "Selection");
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

static const std::set<ActionID> create_group_actions = {
        ActionID::CREATE_GROUP_CHAMFER, ActionID::CREATE_GROUP_FILLET, ActionID::CREATE_GROUP_SKETCH,
        ActionID::CREATE_GROUP_EXTRUDE, ActionID::CREATE_GROUP_LATHE,  ActionID::CREATE_GROUP_LINEAR_ARRAY,
};

static const std::set<ActionID> move_group_actions = {
        ActionID::MOVE_GROUP_UP,
        ActionID::MOVE_GROUP_DOWN,
        ActionID::MOVE_GROUP_TO_END_OF_BODY,
        ActionID::MOVE_GROUP_TO_END_OF_DOCUMENT,
};

void Editor::init_actions()
{
    connect_action(ActionID::SAVE_ALL, [this](auto &a) { m_core.save_all(); });
    connect_action(ActionID::SAVE, [this](auto &a) {
        if (m_core.get_current_idocument_info().has_path()) {
            m_core.save();
            update_version_info();
            if (m_after_save_cb)
                m_after_save_cb();
        }
        else {
            trigger_action(ActionID::SAVE_AS);
        }
    });

    connect_action(ActionID::SAVE_AS, sigc::mem_fun(*this, &Editor::on_save_as));


    connect_action(ActionID::CLOSE_DOCUMENT, [this](auto &a) {
        auto &doci = m_core.get_current_idocument_info();
        close_document(doci.get_uuid(), nullptr, nullptr);
    });


    connect_action(ActionID::NEW_DOCUMENT, [this](auto &a) { m_core.add_document(); });
    connect_action(ActionID::OPEN_DOCUMENT, sigc::mem_fun(*this, &Editor::on_open_document));


    connect_action(ActionID::EXPORT_SOLID_MODEL_STL, sigc::mem_fun(*this, &Editor::on_export_solid_model));
    connect_action(ActionID::EXPORT_SOLID_MODEL_STEP, sigc::mem_fun(*this, &Editor::on_export_solid_model));

    connect_action(ActionID::NEXT_GROUP, [this](auto &a) { m_workspace_browser->group_prev_next(1); });
    connect_action(ActionID::PREVIOUS_GROUP, [this](auto &a) { m_workspace_browser->group_prev_next(-1); });

    connect_action(ActionID::TOGGLE_SOLID_MODEL, [this](const auto &a) {
        auto &doc = m_core.get_current_document();
        auto &group = doc.get_group(m_core.get_current_group());
        auto &body_group = group.find_body(doc).group.m_uuid;

        m_document_view.m_body_views[body_group].m_solid_model_visible =
                !m_document_view.body_solid_model_is_visible(body_group);
        m_workspace_browser->update_current_group(m_document_view);
        canvas_update_keep_selection();
    });

    for (const auto &[id, it] : action_catalog) {
        if (std::holds_alternative<ToolID>(id)) {
            connect_action(std::get<ToolID>(id));
        }
    }

    connect_action(ActionID::UNDO, [this](const auto &a) {
        m_core.undo();
        canvas_update_keep_selection();
        update_workplane_label();
        update_selection_editor();
    });
    connect_action(ActionID::REDO, [this](const auto &a) {
        m_core.redo();
        canvas_update_keep_selection();
        update_workplane_label();
        update_selection_editor();
    });

    connect_action(ActionID::PREFERENCES, [this](const auto &a) {
        auto pwin = dynamic_cast<Dune3DApplication &>(*m_win.get_application()).show_preferences_window();
        pwin->set_transient_for(m_win);
    });


    connect_action(ActionID::VIEW_ALL, [this](auto &a) {
        get_canvas().set_cam_quat(glm::quat_identity<float, glm::defaultp>());
        get_canvas().set_cam_distance(10);
        get_canvas().set_center({0, 0, 0});
    });

    connect_action(ActionID::VIEW_RESET_TILT, [this](auto &a) {
        const auto q = get_canvas().get_cam_quat();
        const auto z = glm::rotate(glm::inverse(q), glm::vec3(0, 0, 1));
        const auto phi = atan2(z.y, z.x);
        const auto ry = glm::angleAxis(-(float)(M_PI / 2 - phi), glm::rotate(q, glm::vec3(0, 0, 1)));
        get_canvas().animate_to_cam_quat(ry * q);
    });

    connect_action(ActionID::ALIGN_VIEW_TO_WORKPLANE, sigc::mem_fun(*this, &Editor::on_align_to_workplane));
    connect_action(ActionID::ALIGN_VIEW_TO_CURRENT_WORKPLANE, sigc::mem_fun(*this, &Editor::on_align_to_workplane));
    connect_action(ActionID::CENTER_VIEW_TO_WORKPLANE, sigc::mem_fun(*this, &Editor::on_center_to_workplane));
    connect_action(ActionID::CENTER_VIEW_TO_CURRENT_WORKPLANE, sigc::mem_fun(*this, &Editor::on_center_to_workplane));
    connect_action(ActionID::ALIGN_AND_CENTER_VIEW_TO_WORKPLANE, [this](const auto &a) {
        trigger_action(ActionID::ALIGN_VIEW_TO_WORKPLANE);
        trigger_action(ActionID::CENTER_VIEW_TO_WORKPLANE);
    });

    connect_action(ActionID::VIEW_PERSP, [this](auto &a) { get_canvas().set_projection(Canvas::Projection::PERSP); });
    connect_action(ActionID::VIEW_ORTHO, [this](auto &a) { get_canvas().set_projection(Canvas::Projection::ORTHO); });
    connect_action(ActionID::VIEW_TOGGLE_PERSP_ORTHO, [this](auto &a) {
        if (get_canvas().get_projection() == Canvas::Projection::PERSP)
            get_canvas().set_projection(Canvas::Projection::ORTHO);
        else
            get_canvas().set_projection(Canvas::Projection::PERSP);
    });

    connect_action(ActionID::DELETE_CURRENT_GROUP, [this](auto &a) { on_delete_current_group(); });

    for (const auto act : create_group_actions) {
        connect_action(act, sigc::mem_fun(*this, &Editor::on_create_group_action));
    }
    for (const auto act : move_group_actions) {
        connect_action(act, sigc::mem_fun(*this, &Editor::on_move_group_action));
    }
    connect_action(ActionID::TOGGLE_WORKPLANE, [this](auto &a) {
        auto &cb = m_win.get_workplane_checkbutton();
        cb.set_active(!cb.get_active());
    });

    connect_action(ActionID::SELECT_PATH, [this](auto &a) {
        auto &doc = m_core.get_current_document();

        auto en_uu = entity_from_selection(doc, get_canvas().get_selection());
        if (!en_uu)
            return;
        auto &en = doc.get_entity(*en_uu);
        auto &group = doc.get_group(en.m_group);
        auto en_wrkpl = dynamic_cast<const IEntityInWorkplane *>(&en);
        if (!en_wrkpl)
            return;
        auto paths = solid_model_util::Paths::from_document(m_core.get_current_document(), en_wrkpl->get_workplane(),
                                                            group.m_uuid);

        for (auto &path : paths.paths) {
            for (auto &[node, edge] : path) {
                if (edge.entity.m_uuid == en.m_uuid) {
                    std::set<SelectableRef> sel;
                    for (auto &[node2, edge2] : path) {
                        sel.emplace(UUID(), SelectableRef::Type::ENTITY, edge2.entity.m_uuid, 0);
                    }
                    get_canvas().set_selection(sel, true);
                    get_canvas().set_selection_mode(SelectionMode::NORMAL);
                    return;
                }
            }
        }
    });

    connect_action(ActionID::EXPORT_PATHS, sigc::mem_fun(*this, &Editor::on_export_paths));
    connect_action(ActionID::EXPORT_PROJECTION, sigc::mem_fun(*this, &Editor::on_export_projection));

    m_core.signal_rebuilt().connect([this] { update_action_sensitivity(); });
}


void Editor::on_align_to_workplane(const ActionConnection &conn)
{
    if (!m_core.has_documents())
        return;
    UUID wrkpl_uu;
    const auto action = std::get<ActionID>(conn.id);

    if (action == ActionID::ALIGN_VIEW_TO_CURRENT_WORKPLANE) {
        wrkpl_uu = m_core.get_current_workplane();
    }
    else {
        if (auto wrkpl_opt = entity_from_selection(m_core.get_current_document(), get_canvas().get_selection(),
                                                   Entity::Type::WORKPLANE))
            wrkpl_uu = *wrkpl_opt;
    }
    if (!wrkpl_uu)
        return;

    auto &wrkpl = m_core.get_current_document().get_entity<EntityWorkplane>(wrkpl_uu);
    get_canvas().animate_to_cam_quat(wrkpl.m_normal);
}

void Editor::on_center_to_workplane(const ActionConnection &conn)
{
    if (!m_core.has_documents())
        return;
    UUID wrkpl_uu;
    const auto action = std::get<ActionID>(conn.id);

    if (action == ActionID::CENTER_VIEW_TO_CURRENT_WORKPLANE) {
        wrkpl_uu = m_core.get_current_workplane();
    }
    else {
        if (auto wrkpl_opt = entity_from_selection(m_core.get_current_document(), get_canvas().get_selection(),
                                                   Entity::Type::WORKPLANE))
            wrkpl_uu = *wrkpl_opt;
    }
    if (!wrkpl_uu)
        return;

    auto &wrkpl = m_core.get_current_document().get_entity<EntityWorkplane>(wrkpl_uu);
    get_canvas().animate_to_center_abs(wrkpl.m_origin);
}


void Editor::on_export_solid_model(const ActionConnection &conn)
{
    const auto action = std::get<ActionID>(conn.id);
    auto dialog = Gtk::FileDialog::create();

    // Add filters, so that only certain file types can be selected:
    auto filters = Gio::ListStore<Gtk::FileFilter>::create();

    auto filter_any = Gtk::FileFilter::create();
    std::string suffix;
    if (action == ActionID::EXPORT_SOLID_MODEL_STEP) {
        filter_any->set_name("STEP");
        filter_any->add_pattern("*.step");
        filter_any->add_pattern("*.stp");
        suffix = ".step";
    }
    else {
        filter_any->set_name("STL");
        filter_any->add_pattern("*.stl");
        suffix = ".stl";
    }
    filters->append(filter_any);

    dialog->set_filters(filters);

    // Show the dialog and wait for a user response:
    dialog->save(m_win, [this, dialog, action, suffix](const Glib::RefPtr<Gio::AsyncResult> &result) {
        try {
            auto file = dialog->save_finish(result);
            // open_file_view(file);
            //  Notice that this is a std::string, not a Glib::ustring.
            const auto path = path_from_string(append_suffix_if_required(file->get_path(), suffix));
            auto &group = m_core.get_current_document().get_group(m_core.get_current_group());
            if (auto gr = dynamic_cast<const IGroupSolidModel *>(&group)) {
                if (action == ActionID::EXPORT_SOLID_MODEL_STEP)
                    gr->get_solid_model()->export_step(path);
                else
                    gr->get_solid_model()->export_stl(path);
            }
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

void Editor::on_export_paths(const ActionConnection &conn)
{
    auto dialog = Gtk::FileDialog::create();

    // Add filters, so that only certain file types can be selected:
    auto filters = Gio::ListStore<Gtk::FileFilter>::create();

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("SVG");
    filter_any->add_pattern("*.svg");
    filters->append(filter_any);

    dialog->set_filters(filters);

    // Show the dialog and wait for a user response:
    dialog->save(m_win, [this, dialog](const Glib::RefPtr<Gio::AsyncResult> &result) {
        try {
            auto file = dialog->save_finish(result);
            // open_file_view(file);
            //  Notice that this is a std::string, not a Glib::ustring.
            const auto path = path_from_string(append_suffix_if_required(file->get_path(), ".svg"));

            auto group_filter = [this](const Group &group) {
                auto &body_group = group.find_body(m_core.get_current_document()).group;
                auto group_visible = m_document_view.group_is_visible(group.m_uuid);
                auto body_visible = m_document_view.body_is_visible(body_group.m_uuid);
                return body_visible && group_visible;
            };
            export_paths(path, m_core.get_current_document(), m_core.get_current_group(), group_filter);
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

void Editor::on_export_projection(const ActionConnection &conn)
{
    auto dialog = Gtk::FileDialog::create();

    // Add filters, so that only certain file types can be selected:
    auto filters = Gio::ListStore<Gtk::FileFilter>::create();

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("SVG");
    filter_any->add_pattern("*.svg");
    filters->append(filter_any);

    dialog->set_filters(filters);

    // Show the dialog and wait for a user response:
    dialog->save(m_win, [this, dialog](const Glib::RefPtr<Gio::AsyncResult> &result) {
        try {
            auto file = dialog->save_finish(result);
            // open_file_view(file);
            //  Notice that this is a std::string, not a Glib::ustring.
            const auto path = path_from_string(append_suffix_if_required(file->get_path(), ".svg"));

            auto sel = get_canvas().get_selection();
            glm::dvec3 origin = get_canvas().get_center();
            glm::dquat normal = get_canvas().get_cam_quat();
            for (const auto &it : sel) {
                if (it.type == SelectableRef::Type::ENTITY) {
                    if (m_core.get_current_document().m_entities.count(it.item)
                        && m_core.get_current_document().m_entities.at(it.item)->get_type()
                                   == Entity::Type::WORKPLANE) {
                        auto &wrkpl = m_core.get_current_document().get_entity<EntityWorkplane>(it.item);
                        origin = wrkpl.m_origin;
                        normal = wrkpl.m_normal;
                        break;
                    }
                }
            }
            auto &group = m_core.get_current_document().get_group(m_core.get_current_group());
            if (auto gr = dynamic_cast<const IGroupSolidModel *>(&group))
                gr->get_solid_model()->export_projection(path, origin, normal);
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

void Editor::on_open_document(const ActionConnection &conn)
{
    auto dialog = Gtk::FileDialog::create();

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

void Editor::on_create_group_action(const ActionConnection &conn)
{
    static const std::map<ActionID, Group::Type> group_types = {
            {ActionID::CREATE_GROUP_CHAMFER, Group::Type::CHAMFER},
            {ActionID::CREATE_GROUP_FILLET, Group::Type::FILLET},
            {ActionID::CREATE_GROUP_EXTRUDE, Group::Type::EXTRUDE},
            {ActionID::CREATE_GROUP_LATHE, Group::Type::LATHE},
            {ActionID::CREATE_GROUP_SKETCH, Group::Type::SKETCH},
            {ActionID::CREATE_GROUP_LINEAR_ARRAY, Group::Type::LINEAR_ARRAY},
    };
    on_add_group(group_types.at(std::get<ActionID>(conn.id)));
}

static const std::map<ActionID, Document::MoveGroup> move_types = {
        {ActionID::MOVE_GROUP_UP, Document::MoveGroup::UP},
        {ActionID::MOVE_GROUP_DOWN, Document::MoveGroup::DOWN},
        {ActionID::MOVE_GROUP_TO_END_OF_BODY, Document::MoveGroup::END_OF_BODY},
        {ActionID::MOVE_GROUP_TO_END_OF_DOCUMENT, Document::MoveGroup::END_OF_DOCUMENT},
};

void Editor::on_move_group_action(const ActionConnection &conn)
{
    on_move_group(move_types.at(std::get<ActionID>(conn.id)));
}

void Editor::on_save_as(const ActionConnection &conn)
{
    auto dialog = Gtk::FileDialog::create();

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
            m_workspace_browser->update_documents(m_document_view);
            update_version_info();
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
                bool r = m_core.tool_can_begin(std::get<ToolID>(id), sel).can_begin;
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


void Editor::show_save_dialog(std::function<void()> save_cb, std::function<void()> no_save_cb)
{
    auto dialog = Gtk::AlertDialog::create("Save changes before closing?");
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
                [this, doc_uu, save_cb, no_save_cb] {
                    m_after_save_cb = [this, doc_uu, save_cb] {
                        m_core.close_document(doc_uu);
                        if (save_cb)
                            save_cb();
                    };
                    trigger_action(ActionID::SAVE);
                },
                [this, doc_uu, no_save_cb] {
                    m_core.close_document(doc_uu);
                    if (no_save_cb)
                        no_save_cb();
                });
    }
    else {
        m_core.close_document(doc_uu);
    }
}

void Editor::update_group_editor()
{
    if (m_group_editor) {
        m_group_editor_box->remove(*m_group_editor);
        m_group_editor = nullptr;
    }
    if (!m_core.has_documents())
        return;
    m_group_editor = GroupEditor::create(m_core, m_core.get_current_group());
    m_group_editor->signal_changed().connect([this] {
        m_core.set_needs_save();
        m_core.rebuild("group edited");
        canvas_update_keep_selection();
    });
    m_group_editor->signal_trigger_action().connect([this](auto act) { trigger_action(act); });
    m_group_editor_box->append(*m_group_editor);
}


void Editor::update_workplane_label()
{
    if (!m_core.has_documents()) {
        m_win.get_workplane_checkbutton().set_label("No documents");
        return;
    }
    auto wrkpl_uu = m_core.get_current_workplane();
    if (!wrkpl_uu) {
        m_win.get_workplane_checkbutton().set_label("No Workplane");
    }
    else {
        auto &wrkpl = m_core.get_current_document().get_entity<EntityWorkplane>(wrkpl_uu);
        auto &wrkpl_group = m_core.get_current_document().get_group<Group>(wrkpl.m_group);
        std::string s = "Workplane ";
        if (wrkpl.m_name.size())
            s += wrkpl.m_name + " ";
        s += "in group " + wrkpl_group.m_name;
        m_win.get_workplane_checkbutton().set_label(s);
    }
}

KeyMatchResult Editor::keys_match(const KeySequence &keys) const
{
    return key_sequence_match(m_keys_current, keys);
}

void Editor::update_action_sensitivity()
{
    update_action_sensitivity(get_canvas().get_selection());
}

void Editor::update_action_sensitivity(const std::set<SelectableRef> &sel)
{
    m_action_sensitivity[ActionID::UNDO] = m_core.can_undo();
    m_action_sensitivity[ActionID::REDO] = m_core.can_redo();
    m_action_sensitivity[ActionID::SAVE_ALL] = m_core.get_needs_save_any();
    m_action_sensitivity[ActionID::SAVE] = m_core.get_needs_save();
    m_action_sensitivity[ActionID::SAVE_AS] = m_core.has_documents();
    m_action_sensitivity[ActionID::CLOSE_DOCUMENT] = m_core.has_documents();
    m_action_sensitivity[ActionID::OPEN_DOCUMENT] = !m_core.has_documents();

    m_action_sensitivity[ActionID::TOGGLE_SOLID_MODEL] = m_core.has_documents();
    m_action_sensitivity[ActionID::NEW_DOCUMENT] = !m_core.has_documents();
    m_action_sensitivity[ActionID::TOGGLE_WORKPLANE] = m_core.has_documents();
    bool has_solid_model = false;

    for (const auto act : create_group_actions) {
        m_action_sensitivity[act] = m_core.has_documents();
    }

    for (const auto act : move_group_actions) {
        m_action_sensitivity[act] = m_core.has_documents();
    }
    if (m_core.has_documents()) {
        auto &current_group = m_core.get_current_document().get_group(m_core.get_current_group());
        has_solid_model = dynamic_cast<const IGroupSolidModel *>(&current_group);
        auto groups_sorted = m_core.get_current_document().get_groups_sorted();
        assert(groups_sorted.size());
        const bool is_first = groups_sorted.front() == &current_group;
        const bool is_last = groups_sorted.back() == &current_group;
        m_action_sensitivity[ActionID::PREVIOUS_GROUP] = !is_first;
        m_action_sensitivity[ActionID::NEXT_GROUP] = !is_last;
        m_action_sensitivity[ActionID::DELETE_CURRENT_GROUP] = !is_first;
        const auto has_current_wrkpl = current_group.m_active_wrkpl != UUID();
        m_action_sensitivity[ActionID::CREATE_GROUP_EXTRUDE] = has_current_wrkpl;
        m_action_sensitivity[ActionID::CREATE_GROUP_LATHE] = has_current_wrkpl;
        for (const auto &[act, mg] : move_types) {
            m_action_sensitivity[act] =
                    m_core.get_current_document().get_group_after(current_group.m_uuid, mg) != UUID();
        }
        m_action_sensitivity[ActionID::ALIGN_VIEW_TO_CURRENT_WORKPLANE] = has_current_wrkpl;
        m_action_sensitivity[ActionID::CENTER_VIEW_TO_CURRENT_WORKPLANE] = has_current_wrkpl;
        const auto sel_is_workplane =
                entity_from_selection(m_core.get_current_document(), sel, Entity::Type::WORKPLANE).has_value();
        m_action_sensitivity[ActionID::ALIGN_VIEW_TO_WORKPLANE] = sel_is_workplane;
        m_action_sensitivity[ActionID::CENTER_VIEW_TO_WORKPLANE] = sel_is_workplane;
        m_action_sensitivity[ActionID::ALIGN_AND_CENTER_VIEW_TO_WORKPLANE] = sel_is_workplane;
        m_action_sensitivity[ActionID::SELECT_PATH] =
                entity_from_selection(m_core.get_current_document(), sel).has_value();
        m_action_sensitivity[ActionID::EXPORT_PATHS] = has_current_wrkpl;
    }
    else {
        m_action_sensitivity[ActionID::PREVIOUS_GROUP] = false;
        m_action_sensitivity[ActionID::NEXT_GROUP] = false;
        m_action_sensitivity[ActionID::DELETE_CURRENT_GROUP] = false;
        m_action_sensitivity[ActionID::ALIGN_VIEW_TO_WORKPLANE] = false;
        m_action_sensitivity[ActionID::ALIGN_VIEW_TO_CURRENT_WORKPLANE] = false;
        m_action_sensitivity[ActionID::CENTER_VIEW_TO_WORKPLANE] = false;
        m_action_sensitivity[ActionID::CENTER_VIEW_TO_CURRENT_WORKPLANE] = false;
        m_action_sensitivity[ActionID::SELECT_PATH] = false;
        m_action_sensitivity[ActionID::EXPORT_PATHS] = false;
    }

    m_action_sensitivity[ActionID::EXPORT_SOLID_MODEL_STEP] = has_solid_model;
    m_action_sensitivity[ActionID::EXPORT_SOLID_MODEL_STL] = has_solid_model;
    m_action_sensitivity[ActionID::EXPORT_PROJECTION] = has_solid_model;


    m_signal_action_sensitive.emit();
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

    m_win.tool_bar_set_vertical(m_preferences.tool_bar.vertical_layout);
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

Gtk::Button *Editor::create_action_button(ActionToolID action)
{
    auto &catitem = action_catalog.at(action);
    auto button = Gtk::manage(new Gtk::Button(catitem.name));
    attach_action_button(*button, action);
    return button;
}

bool Editor::trigger_action(ActionToolID action, ActionSource source)
{
    if (m_core.tool_is_active() && !(action_catalog.at(action).flags & ActionCatalogItem::FLAGS_IN_TOOL)) {
        return false;
    }
    if (std::holds_alternative<ActionID>(action) && !get_action_sensitive(std::get<ActionID>(action)))
        return false;
    /*main_window->key_hint_set_visible(false);
    if (keys_current.size()) {
        keys_current.clear();
        reset_tool_hint_label();
    }*/
    auto conn = m_action_connections.at(action);
    conn.cb(conn, source);
    return true;
}

bool Editor::get_action_sensitive(ActionID action) const
{
    if (!m_action_connections.count(action)) // actions not connected can't be sensitive
        return false;

    if (m_core.tool_is_active()) // actions available int tools are always sensitive
        return action_catalog.at(action).flags & ActionCatalogItem::FLAGS_IN_TOOL;

    if (m_action_sensitivity.count(action))
        return m_action_sensitivity.at(action);
    else
        return true;
}

void Editor::attach_action_button(Gtk::Button &button, ActionToolID action)
{
    button.signal_clicked().connect([this, action] { trigger_action(action); });
    attach_action_sensitive(button, action);
}

void Editor::attach_action_sensitive(Gtk::Widget &widget, ActionToolID action)
{
    if (std::holds_alternative<ActionID>(action)) {
        m_signal_action_sensitive.connect(
                [this, &widget, action] { widget.set_sensitive(get_action_sensitive(std::get<ActionID>(action))); });
        widget.set_sensitive(get_action_sensitive(std::get<ActionID>(action)));
    }
}

void Editor::handle_tool_action(const ActionConnection &conn)
{
    tool_begin(std::get<ToolID>(conn.id));
}

ActionConnection &Editor::connect_action(ToolID tool_id)
{
    return connect_action(tool_id, sigc::mem_fun(*this, &Editor::handle_tool_action));
}

ActionConnection &Editor::connect_action_with_source(ActionToolID id,
                                                     std::function<void(const ActionConnection &, ActionSource)> cb)
{
    if (m_action_connections.count(id)) {
        throw std::runtime_error("duplicate action");
    }
    if (action_catalog.count(id) == 0) {
        throw std::runtime_error("invalid action");
    }
    auto &act = m_action_connections
                        .emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(id, cb))
                        .first->second;

    return act;
}

ActionConnection &Editor::connect_action(ActionToolID id, std::function<void(const ActionConnection &)> cb)
{
    return connect_action_with_source(id, [cb](const ActionConnection &conn, ActionSource) { cb(conn); });
}

void Editor::canvas_update()
{
    auto docs = m_core.get_documents();
    auto hover_sel = get_canvas().get_hover_selection();
    get_canvas().clear();
    for (const auto doc : docs) {
        Renderer renderer(get_canvas());
        renderer.m_solid_model_edge_select_mode = m_solid_model_edge_select_mode;
        renderer.render(doc->get_document(), doc->get_current_group(), m_document_view);
    }
    get_canvas().set_hover_selection(hover_sel);
    get_canvas().request_push();
}

void Editor::canvas_update_keep_selection()
{
    auto sel = get_canvas().get_selection();
    canvas_update();
    get_canvas().set_selection(sel, false);
}

void Editor::tool_begin(ToolID id /*bool override_selection, const std::set<SelectableRef> &sel,
                         std::unique_ptr<ToolData> data*/)
{
    if (m_core.tool_is_active()) {
        Logger::log_critical("can't begin tool while tool is active", Logger::Domain::EDITOR);
        return;
    }

    ToolArgs args;
    // args.data = std::move(data);

    //    if (override_selection)
    //        args.selection = sel;
    //   else
    args.selection = get_canvas().get_selection();
    m_last_selection_mode = get_canvas().get_selection_mode();
    get_canvas().set_selection_mode(SelectionMode::NONE);
    ToolResponse r = m_core.tool_begin(id, args);
    tool_process(r);
}


void Editor::tool_update_data(std::unique_ptr<ToolData> data)
{
    if (m_core.tool_is_active()) {
        ToolArgs args;
        args.type = ToolEventType::DATA;
        args.data = std::move(data);
        ToolResponse r = m_core.tool_update(args);
        tool_process(r);
    }
}

void Editor::enable_hover_selection()
{
    get_canvas().set_selection_mode(SelectionMode::HOVER_ONLY);
}

std::optional<SelectableRef> Editor::get_hover_selection() const
{
    return get_canvas().get_hover_selection();
}

void Editor::tool_process(ToolResponse &resp)
{
    tool_process_one();
    while (auto args = m_core.get_pending_tool_args()) {
        m_core.tool_update(*args);

        tool_process_one();
    }
}

void Editor::canvas_update_from_tool()
{
    canvas_update();
    get_canvas().set_selection(m_core.get_tool_selection(), false);
}


void Editor::tool_process_one()
{
    if (!m_core.tool_is_active()) {
        m_dialogs.close_nonmodal();
        // imp_interface->dialogs.close_nonmodal();
        // reset_tool_hint_label();
        // canvas->set_cursor_external(false);
        // canvas->snap_filter.clear();
        m_no_canvas_update = false;
        m_solid_model_edge_select_mode = false;
        update_workplane_label();
        update_selection_editor();
    }
    if (!m_no_canvas_update)
        canvas_update();
    get_canvas().set_selection(m_core.get_tool_selection(), false);
    if (!m_core.tool_is_active())
        get_canvas().set_selection_mode(m_last_selection_mode);

    /*  if (m_core.tool_is_active()) {
          canvas->set_selection(m_core.get_tool_selection());
      }
      else {
          canvas->set_selection(m_core.get_tool_selection(),
                                canvas->get_selection_mode() == CanvasGL::SelectionMode::NORMAL);
      }*/
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
        std::cout << "tool cursor move" << std::endl;
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

std::optional<ActionToolID> Editor::get_doubleclick_action(const SelectableRef &sr)
{
    if (sr.type == SelectableRef::Type::CONSTRAINT) {
        auto &constraint = m_core.get_current_document().get_constraint(sr.item);
        if (dynamic_cast<IConstraintDatum *>(&constraint))
            return ToolID::ENTER_DATUM;
    }
    else if (sr.type == SelectableRef::Type::ENTITY) {
        auto &entity = m_core.get_current_document().get_entity(sr.item);
        switch (entity.get_type()) {
        case Entity::Type::LINE_2D:
        case Entity::Type::ARC_2D:
            if (sr.point == 1 || sr.point == 2)
                return ToolID::DRAW_CONTOUR_FROM_POINT;
            break;
        case Entity::Type::WORKPLANE:
            return ActionID::ALIGN_AND_CENTER_VIEW_TO_WORKPLANE;
        default:;
        }
    }
    return {};
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


bool Editor::handle_action_key(Glib::RefPtr<Gtk::EventControllerKey> controller, unsigned int keyval,
                               Gdk::ModifierType state)
{
    auto ev = controller->get_current_event();
    if (ev->is_modifier())
        return false;
    state &= ~ev->get_consumed_modifiers();
    state &= (Gdk::ModifierType::SHIFT_MASK | Gdk::ModifierType::CONTROL_MASK | Gdk::ModifierType::ALT_MASK);
    if (keyval == GDK_KEY_Escape) {
        if (!m_core.tool_is_active()) {
            get_canvas().set_selection_mode(SelectionMode::HOVER);
            get_canvas().set_selection({}, true);

            reset_key_hint_label();
            if (m_keys_current.size() == 0) {
                return false;
            }
            else {
                // main_window->key_hint_set_visible(false);
                m_keys_current.clear();
                return true;
            }
        }
        else {
            ToolArgs args;
            args.type = ToolEventType::ACTION;
            args.action = InToolActionID::CANCEL;
            ToolResponse r = m_core.tool_update(args);
            tool_process(r);
            return true;
        }
    }
    else {
        m_keys_current.push_back({keyval, state});
        auto in_tool_actions = m_core.get_tool_actions();
        std::map<InToolActionID, std::pair<KeyMatchResult, KeySequence>> in_tool_actions_matched;
        std::map<ActionConnection *, std::pair<KeyMatchResult, KeySequence>> connections_matched;
        auto selection = get_canvas().get_selection();
        update_action_sensitivity();
        for (auto &[id, conn] : m_action_connections) {
            bool can_begin = false;
            if (std::holds_alternative<ToolID>(id) && !m_core.tool_is_active()) {
                can_begin = m_core.tool_can_begin(std::get<ToolID>(id), selection).can_begin;
            }
            else if (std::holds_alternative<ActionID>(id)) {
                can_begin = get_action_sensitive(std::get<ActionID>(id));
            }
            if (can_begin) {
                for (const auto &it2 : conn.key_sequences) {
                    if (const auto m = keys_match(it2); m != KeyMatchResult::NONE) {
                        connections_matched.emplace(std::piecewise_construct, std::forward_as_tuple(&conn),
                                                    std::forward_as_tuple(m, it2));
                    }
                }
            }
        }
        if (in_tool_actions.size()) {
            for (const auto &[action, seqs] : m_in_tool_key_sequeces_preferences.keys) {
                if (in_tool_actions.count(action)) {
                    for (const auto &seq : seqs) {
                        if (const auto m = keys_match(seq); m != KeyMatchResult::NONE)
                            in_tool_actions_matched.emplace(std::piecewise_construct, std::forward_as_tuple(action),
                                                            std::forward_as_tuple(m, seq));
                    }
                }
            }
        }

        if (connections_matched.size() == 1 && in_tool_actions_matched.size() == 1) {
            m_win.set_key_hint_label_text("Ambiguous");
            m_keys_current.clear();
        }
        else if (connections_matched.size() == 1) {
            m_win.set_key_hint_label_text(key_sequence_to_string(m_keys_current));
            m_keys_current.clear();
            // main_window->key_hint_set_visible(false);
            auto conn = connections_matched.begin()->first;
            if (!trigger_action(conn->id, ActionSource::KEY)) {
                reset_key_hint_label();
                return false;
            }
            return true;
        }
        else if (in_tool_actions_matched.size() == 1) {
            m_win.set_key_hint_label_text(key_sequence_to_string(m_keys_current));
            m_keys_current.clear();

            ToolArgs args;
            args.type = ToolEventType::ACTION;
            args.action = in_tool_actions_matched.begin()->first;
            ToolResponse r = m_core.tool_update(args);
            tool_process(r);

            return true;
        }
        else if (connections_matched.size() > 1 || in_tool_actions_matched.size() > 1) { // still ambiguous
            std::list<std::pair<std::string, KeySequence>> conflicts;
            bool have_conflict = false;
            for (const auto &[conn, it] : connections_matched) {
                const auto &[res, seq] = it;
                if (res == KeyMatchResult::COMPLETE) {
                    have_conflict = true;
                }
                conflicts.emplace_back(action_catalog.at(conn->id).name, seq);
            }
            (void)have_conflict;
            /* for (const auto &[act, it] : in_tool_actions_matched) {
                 const auto &[res, seq] = it;
                 if (res == KeyMatchResult::COMPLETE) {
                     have_conflict = true;
                 }
                 conflicts.emplace_back(in_tool_action_catalog.at(act).name + " (in-tool action)", seq);
             }
             if (have_conflict) {
                 m_win.set_key_hint_label_text("Key sequences conflict");
                 keys_current.clear();
                 KeyConflictDialog dia(main_window, conflicts);
                 if (dia.run() == KeyConflictDialog::RESPONSE_PREFS) {
                     show_preferences("keys");
                 }
                 return false;
             }

             for (auto ch : main_window->key_hint_box->get_children()) {
                 delete ch;
             }

             for (const auto &[conn, it] : connections_matched) {
                 const auto &[res, seq] = it;
                 std::string seq_label;
                 for (size_t i = 0; i < seq.size(); i++) {
                     seq_label += Glib::Markup::escape_text(key_sequence_item_to_string(seq.at(i))) + " ";
                     if (i + 1 == keys_current.size()) {
                         seq_label += "<b>";
                     }
                 }
                 rtrim(seq_label);
                 seq_label += "</b>";

                 auto la = Gtk::manage(new KeyLabel(main_window->key_hint_size_group, seq_label, conn->id));
                 main_window->key_hint_box->append(*la);
                 la->show();
             }
             main_window->key_hint_set_visible(true);
 */
            m_win.set_key_hint_label_text(key_sequence_to_string(m_keys_current) + "?");
            return true;
        }
        else if (connections_matched.size() == 0 || in_tool_actions_matched.size() == 0) {
            m_win.set_key_hint_label_text("Unknown key sequence");
            m_keys_current.clear();
            // main_window->key_hint_set_visible(false);
            return false;
        }
        else {
            Logger::log_warning("Key sequence??", Logger::Domain::EDITOR,
                                std::to_string(connections_matched.size()) + " "
                                        + std::to_string(in_tool_actions_matched.size()));
            return false;
        }
    }
    return false;
}


void Editor::handle_tool_change()
{
    const auto tool_id = m_core.get_tool_id();
    // panels->set_sensitive(id == ToolID::NONE);
    // canvas->set_selection_allowed(id == ToolID::NONE);
    // main_window->tool_bar_set_use_actions(core->get_tool_actions().size());
    if (tool_id != ToolID::NONE) {
        m_win.tool_bar_set_tool_name(action_catalog.at(tool_id).name);
        tool_bar_set_tool_tip("");
    }
    m_win.tool_bar_set_visible(tool_id != ToolID::NONE);
    tool_bar_clear_actions();
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

void Editor::open_file(const std::filesystem::path &path)
{
    if (m_core.has_documents())
        return;
    m_core.add_document(path);
    m_win.get_app().add_recent_item(path);
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

} // namespace dune3d

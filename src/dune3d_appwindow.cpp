#include "dune3d_application.hpp"
#include "dune3d_appwindow.hpp"
#include "canvas/canvas.hpp"
#include "document/document.hpp"
#include "document/entity_line3d.hpp"
#include "document/entity_workplane.hpp"
#include "document/entity_line2d.hpp"
#include "document/all_groups.hpp"
#include "document/constraint.hpp"
#include "document/solid_model.hpp"
#include "render/renderer.hpp"
#include "nlohmann/json.hpp"
#include "logger/logger.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include "in_tool_action/in_tool_action_catalog.hpp"
#include "core/tool_id.hpp"
#include "canvas/selectable_ref.hpp"
#include <iostream>
#include <iomanip>
#include "preferences/preferences.hpp"
#include "action/action_id.hpp"
#include "util/action_label.hpp"
#include "tool_popover.hpp"
#include "workspace_browser.hpp"
#include "constraints_box.hpp"
#include "group_editor.hpp"
#include "util/util.hpp"
#include "util/selection_util.hpp"
#include "preferences/preferences_window.hpp"
#include "widgets/axes_lollipop.hpp"


namespace dune3d {

Dune3DAppWindow *Dune3DAppWindow::create(Dune3DApplication &app)
{
    // Load the Builder file and instantiate its widgets.

    auto refBuilder = Gtk::Builder::create_from_resource("/org/dune3d/dune3d/window.ui");

    auto window = Gtk::Builder::get_widget_derived<Dune3DAppWindow>(refBuilder, "window", app);

    if (!window)
        throw std::runtime_error("No \"window\" object in window.ui");
    return window;
}

Dune3DAppWindow::~Dune3DAppWindow() = default;

Dune3DAppWindow::Dune3DAppWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refBuilder,
                                 class Dune3DApplication &app)
    : Gtk::ApplicationWindow(cobject), m_preferences(app.get_preferences()), m_core(*this), m_dialogs(*this, *this)
{
    m_canvas = Gtk::make_managed<Canvas>();
    m_drag_tool = ToolID::NONE;

    {
        auto paned = refBuilder->get_widget<Gtk::Paned>("paned");
        paned->set_shrink_start_child(false);
    }

    m_left_bar = refBuilder->get_widget<Gtk::Paned>("left_bar");
    m_workspace_browser = Gtk::make_managed<WorkspaceBrowser>(m_core);
    m_left_bar->set_start_child(*m_workspace_browser);

    m_properties_notebook = Gtk::make_managed<Gtk::Notebook>();
    m_properties_notebook->set_show_border(false);
    m_properties_notebook->set_tab_pos(Gtk::PositionType::BOTTOM);
    m_left_bar->set_end_child(*m_properties_notebook);
    {
        m_group_editor_box = Gtk::make_managed<Gtk::Box>();
        m_properties_notebook->append_page(*m_group_editor_box, "Group");
    }


    m_constraints_box = Gtk::make_managed<ConstraintsBox>(m_core);
    m_properties_notebook->append_page(*m_constraints_box, "Constraints");
    m_core.signal_rebuilt().connect([this] { m_constraints_box->update(); });
    m_core.signal_needs_save().connect([this] { update_action_sensitivity(); });
    m_constraints_box->signal_changed().connect([this] { canvas_update_keep_selection(); });

    {
        auto controller = Gtk::EventControllerKey::create();
        controller->signal_key_pressed().connect(
                [this](guint keyval, guint keycode, Gdk::ModifierType state) -> bool {
                    return handle_action_key(keyval, state);
                },
                true);

        add_controller(controller);
    }

    canvas_update();
    m_constraints_box->update();


    // auto pick_button = refBuilder->get_widget<Gtk::Button>("pick_button");
    // pick_button->signal_clicked().connect([this] { m_canvas->queue_pick(); });

    auto titlebar = refBuilder->get_widget<Gtk::HeaderBar>("titlebar");

    /*{
        auto save_button = create_action_button(ActionID::SAVE_ALL);
        titlebar->pack_start(*save_button);
    }*/

    {
        auto button = refBuilder->get_widget<Gtk::Button>("open_button");
        attach_action_button(*button, ActionID::OPEN_DOCUMENT);
    }

    {
        auto button = refBuilder->get_widget<Gtk::Button>("new_button");
        attach_action_button(*button, ActionID::NEW_DOCUMENT);
    }
    {
        auto button = refBuilder->get_widget<Gtk::Button>("save_button");
        attach_action_button(*button, ActionID::SAVE);
    }
    {
        auto button = refBuilder->get_widget<Gtk::Button>("save_as_button");
        attach_action_button(*button, ActionID::SAVE_AS);
    }
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

        titlebar->pack_start(*undo_redo_box);
    }

    {
        auto hamburger_menu_button = refBuilder->get_widget<Gtk::MenuButton>("hamburger_menu_button");
        auto top = Gio::Menu::create();

        top->append_item(Gio::MenuItem::create("Preferences", "app.preferences"));
        top->append_item(Gio::MenuItem::create("About", "app.about"));

        hamburger_menu_button->set_menu_model(top);
    }

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
    connect_action(ActionID::SAVE_AS, [this](auto &a) {
        auto dialog = Gtk::FileDialog::create();

        // Add filters, so that only certain file types can be selected:
        auto filters = Gio::ListStore<Gtk::FileFilter>::create();

        auto filter_any = Gtk::FileFilter::create();
        filter_any->set_name("Dune 3D documents");
        filter_any->add_pattern("*.d3ddoc");
        filters->append(filter_any);

        dialog->set_filters(filters);

        // Show the dialog and wait for a user response:
        dialog->save(*this, [this, dialog](const Glib::RefPtr<Gio::AsyncResult> &result) {
            try {
                auto file = dialog->save_finish(result);
                // open_file_view(file);
                //  Notice that this is a std::string, not a Glib::ustring.
                auto filename = file->get_path();
                // std::cout << "File selected: " << filename << std::endl;
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
    });
    connect_action(ActionID::CLOSE_DOCUMENT, [this](auto &a) {
        auto &doci = m_core.get_current_idocument_info();
        close_document(doci.get_uuid(), nullptr, nullptr);
    });
    m_workspace_browser->signal_close_document().connect(
            [this](const UUID &doc_uu) { close_document(doc_uu, nullptr, nullptr); });

    signal_close_request().connect(
            [this] {
                if (!m_core.get_needs_save_any())
                    return false;

                auto cb = [this] {
                    // here, the close dialog is still there and closing the main window causes a near-segfault
                    // so break out of the current event
                    Glib::signal_idle().connect_once([this] { close(); });
                };
                close_document(m_core.get_current_idocument_info().get_uuid(), cb, cb);

                return true; // keep window open
            },
            true);


    connect_action(ActionID::NEW_DOCUMENT, [this](auto &a) { m_core.add_document(); });
    connect_action(ActionID::OPEN_DOCUMENT, [this](auto &a) {
        auto dialog = Gtk::FileDialog::create();

        // Add filters, so that only certain file types can be selected:
        auto filters = Gio::ListStore<Gtk::FileFilter>::create();

        auto filter_any = Gtk::FileFilter::create();
        filter_any->set_name("Dune 3D documents");
        filter_any->add_pattern("*.d3ddoc");
        filters->append(filter_any);

        dialog->set_filters(filters);

        // Show the dialog and wait for a user response:
        dialog->open(*this, [this, dialog](const Glib::RefPtr<Gio::AsyncResult> &result) {
            try {
                auto file = dialog->open_finish(result);
                open_file_view(file);
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
    });


    connect_action(ActionID::EXPORT_SOLID_MODEL_STL, [this](auto &a) {
        auto dialog = Gtk::FileDialog::create();

        // Add filters, so that only certain file types can be selected:
        auto filters = Gio::ListStore<Gtk::FileFilter>::create();

        auto filter_any = Gtk::FileFilter::create();
        filter_any->set_name("STL");
        filter_any->add_pattern("*.stl");
        filters->append(filter_any);

        dialog->set_filters(filters);

        // Show the dialog and wait for a user response:
        dialog->save(*this, [this, dialog](const Glib::RefPtr<Gio::AsyncResult> &result) {
            try {
                auto file = dialog->save_finish(result);
                // open_file_view(file);
                //  Notice that this is a std::string, not a Glib::ustring.

                auto &group = m_core.get_current_document().get_group(m_core.get_current_group());
                if (auto gr = dynamic_cast<const IGroupSolidModel *>(&group))
                    gr->get_solid_model()->export_stl(file->get_path());
            }
            catch (const Gtk::DialogError &err) {
                // Can be thrown by dialog->open_finish(result).
                std::cout << "No file selected. " << err.what() << std::endl;
            }
            catch (const Glib::Error &err) {
                std::cout << "Unexpected exception. " << err.what() << std::endl;
            }
        });
    });
    connect_action(ActionID::EXPORT_SOLID_MODEL_STEP, [this](auto &a) {
        auto dialog = Gtk::FileDialog::create();

        // Add filters, so that only certain file types can be selected:
        auto filters = Gio::ListStore<Gtk::FileFilter>::create();

        auto filter_any = Gtk::FileFilter::create();
        filter_any->set_name("STEP");
        filter_any->add_pattern("*.step");
        filter_any->add_pattern("*.stp");
        filters->append(filter_any);

        dialog->set_filters(filters);

        // Show the dialog and wait for a user response:
        dialog->save(*this, [this, dialog](const Glib::RefPtr<Gio::AsyncResult> &result) {
            try {
                auto file = dialog->save_finish(result);
                // open_file_view(file);
                //  Notice that this is a std::string, not a Glib::ustring.

                auto &group = m_core.get_current_document().get_group(m_core.get_current_group());
                if (auto gr = dynamic_cast<const IGroupSolidModel *>(&group))
                    gr->get_solid_model()->export_step(file->get_path());
            }
            catch (const Gtk::DialogError &err) {
                // Can be thrown by dialog->open_finish(result).
                std::cout << "No file selected. " << err.what() << std::endl;
            }
            catch (const Glib::Error &err) {
                std::cout << "Unexpected exception. " << err.what() << std::endl;
            }
        });
    });

    connect_action(ActionID::NEXT_GROUP, [this](auto &a) { m_workspace_browser->group_prev_next(1); });
    connect_action(ActionID::PREVIOUS_GROUP, [this](auto &a) { m_workspace_browser->group_prev_next(-1); });

    {
        auto controller = Gtk::EventControllerMotion::create();
        controller->signal_motion().connect([this](double x, double y) {
            if (m_last_x != x || m_last_y != y) {
                handle_cursor_move();
                m_last_x = x;
                m_last_y = y;
            }
        });
        m_canvas->add_controller(controller);
    }

    connect_action(ActionID::TOGGLE_SOLID_MODEL, [this](const auto &a) {
        auto &doc = m_core.get_current_document();
        auto &group = *doc.m_groups.at(m_core.get_current_group());
        auto &body_group = group.find_body(doc).group.m_uuid;

        m_document_view.m_body_views[body_group].m_solid_model_visible =
                !m_document_view.body_solid_model_is_visible(body_group);
        m_workspace_browser->update_current_group(m_document_view);
        canvas_update_keep_selection();
    });

    {
        auto controller = Gtk::GestureClick::create();
        controller->set_button(0);
        controller->signal_pressed().connect([this, controller](int n_press, double x, double y) {
            auto button = controller->get_current_button();
            std::cout << "click " << button << std::endl;
            if (button == 1 || button == 3)
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
        });

        m_canvas->add_controller(controller);
    }


    m_tool_bar = refBuilder->get_widget<Gtk::Revealer>("tool_bar");
    m_tool_bar_actions_box = refBuilder->get_widget<Gtk::Box>("tool_bar_actions_box");
    m_tool_bar_box = refBuilder->get_widget<Gtk::Box>("tool_bar_box");
    m_tool_bar_name_label = refBuilder->get_widget<Gtk::Label>("tool_bar_name_label");
    m_tool_bar_tip_label = refBuilder->get_widget<Gtk::Label>("tool_bar_tip_label");
    m_tool_bar_flash_label = refBuilder->get_widget<Gtk::Label>("tool_bar_flash_label");
    m_tool_bar_stack = refBuilder->get_widget<Gtk::Stack>("tool_bar_stack");
    m_tool_bar_stack->set_visible_child(*m_tool_bar_box);

    refBuilder->get_widget<Gtk::Box>("canvas_box")->insert_child_at_start(*m_canvas);
    m_canvas->set_vexpand(true);
    m_canvas->set_hexpand(true);
    m_key_hint_label = refBuilder->get_widget<Gtk::Label>("key_hint_label");
    m_workplane_label = refBuilder->get_widget<Gtk::Label>("workplane_label");
    update_workplane_label();

    for (const auto &[id, it] : action_catalog) {
        if (std::holds_alternative<ToolID>(id)) {
            connect_action(std::get<ToolID>(id));
        }
    }

    connect_action(ActionID::UNSET_WORKPLANE, [this](const auto &conn) {
        m_core.set_current_workplane(UUID());
        update_workplane_label();
    });
    connect_action(ActionID::SET_WORKPLANE, [this](const auto &conn) {
        auto sel = m_canvas->get_selection();
        for (const auto &it : sel) {
            if (it.type == SelectableRef::Type::ENTITY) {
                if (m_core.get_current_document().m_entities.count(it.item)
                    && m_core.get_current_document().m_entities.at(it.item)->get_type() == Entity::Type::WORKPLANE) {
                    m_core.set_current_workplane(it.item);
                    break;
                }
            }
        }
        update_workplane_label();
    });

    m_tool_popover = Gtk::make_managed<ToolPopover>();
    m_tool_popover->set_parent(*m_canvas);
    m_tool_popover->signal_action_activated().connect([this](ActionToolID action_id) { trigger_action(action_id); });


    connect_action({ActionID::POPOVER}, [this](const auto &a) {
        Gdk::Rectangle rect;
        rect.set_x(m_last_x);
        rect.set_y(m_last_y);

        m_tool_popover->set_pointing_to(rect);

        this->update_action_sensitivity();
        std::map<ActionToolID, bool> can_begin;
        auto sel = m_canvas->get_selection();
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

    connect_action(ActionID::UNDO, [this](const auto &a) {
        m_core.undo();
        canvas_update_keep_selection();
    });
    connect_action(ActionID::REDO, [this](const auto &a) {
        m_core.redo();
        canvas_update_keep_selection();
    });

    connect_action(ActionID::PREFERENCES, [this](const auto &a) {
        auto pwin = dynamic_cast<Dune3DApplication &>(*get_application()).show_preferences_window();
        pwin->set_transient_for(*this);
    });


    m_core.signal_rebuilt().connect([this] { update_action_sensitivity(); });

    m_preferences.signal_changed().connect(sigc::mem_fun(*this, &Dune3DAppWindow::apply_preferences));

    apply_preferences();

    m_core.signal_tool_changed().connect(sigc::mem_fun(*this, &Dune3DAppWindow::handle_tool_change));

    m_workspace_browser->update_documents(m_document_view);

    m_workspace_browser->signal_group_selected().connect([this](const UUID &uu_doc, const UUID &uu_group) {
        if (m_core.tool_is_active())
            return;
        if (m_core.get_current_idocument_info().get_uuid() == uu_doc) {
            if (m_core.get_current_group() == uu_group)
                return;
            m_core.set_current_group(uu_group);
            m_workspace_browser->update_current_group(m_document_view);
            canvas_update_keep_selection();
            update_workplane_label();
            m_constraints_box->update();
            update_group_editor();
            update_action_sensitivity();
        }
    });
    m_workspace_browser->signal_add_group().connect([this](Group::Type group_type) {
        if (m_core.tool_is_active())
            return;
        auto &doc = m_core.get_current_document();
        auto &current_group = doc.get_group(m_core.get_current_group());
        Group *new_group = nullptr;
        if (group_type == Group::Type::SKETCH) {
            auto &group = doc.insert_group<GroupSketch>(UUID::random(), current_group.m_uuid);
            new_group = &group;
            group.m_name = "Sketch";
        }
        else if (group_type == Group::Type::EXTRUDE) {
            if (!current_group.m_active_wrkpl)
                return;
            auto &group = doc.insert_group<GroupExtrude>(UUID::random(), current_group.m_uuid);
            new_group = &group;
            group.m_name = "Extrude";
            group.m_wrkpl = current_group.m_active_wrkpl;
            group.m_source_group = current_group.m_uuid;
        }
        else if (group_type == Group::Type::LATHE) {
            if (!current_group.m_active_wrkpl)
                return;
            auto sel = m_canvas->get_selection();
            std::optional<UUID> wrkpl = entity_from_selection(doc, sel, Entity::Type::WORKPLANE);
            std::optional<LineAndPoint> line_and_point =
                    line_and_point_from_selection(doc, sel, LineAndPoint::AllowSameEntity::YES);
            if (!wrkpl && !line_and_point)
                return;

            auto &group = doc.insert_group<GroupLathe>(UUID::random(), current_group.m_uuid);
            new_group = &group;
            group.m_name = "Lathe";
            group.m_wrkpl = current_group.m_active_wrkpl;
            group.m_source_group = current_group.m_uuid;
            if (wrkpl) {
                group.m_origin = *wrkpl;
                group.m_origin_point = 1;
                group.m_normal = *wrkpl;
            }
            else {
                assert(line_and_point.has_value());
                group.m_origin = line_and_point->point;
                group.m_origin_point = line_and_point->point_point;
                group.m_normal = line_and_point->line;
            }
        }
        else if (group_type == Group::Type::FILLET) {
            auto &group = doc.insert_group<GroupFillet>(UUID::random(), current_group.m_uuid);
            new_group = &group;
            group.m_name = "Fillet";
        }
        else if (group_type == Group::Type::CHAMFER) {
            auto &group = doc.insert_group<GroupChamfer>(UUID::random(), current_group.m_uuid);
            new_group = &group;
            group.m_name = "Chamfer";
            m_core.set_needs_save();
        }
        if (new_group) {
            m_core.set_needs_save();
            m_core.rebuild("add group");
            m_workspace_browser->update_documents(m_document_view);
            canvas_update_keep_selection();
            m_workspace_browser->select_group(new_group->m_uuid);
        }
    });
    m_workspace_browser->signal_delete_current_group().connect([this] {
        if (m_core.tool_is_active())
            return;

        auto &doc = m_core.get_current_document();

        auto &group = doc.get_group(m_core.get_current_group());
        if (group.get_type() == Group::Type::REFERENCE)
            return;

        UUID previous_group;
        previous_group = doc.get_group_rel(group.m_uuid, -1);
        if (!previous_group)
            previous_group = doc.get_group_rel(group.m_uuid, 1);

        if (!previous_group)
            return;

        {
            Document::ItemsToDelete items;
            items.groups = {group.m_uuid};
            auto exra_items = doc.get_additional_items_to_delete(items);
            items.append(exra_items);
            doc.delete_items(items);
        }

        m_core.set_current_group(previous_group);

        m_core.set_needs_save();
        m_core.rebuild("delete group");
        canvas_update_keep_selection();
        m_workspace_browser->update_documents(m_document_view);
    });


    m_workspace_browser->signal_move_group().connect([this](WorkspaceBrowser::MoveGroup op) {
        if (m_core.tool_is_active())
            return;
        auto &doc = m_core.get_current_document();

        auto &group = doc.get_group(m_core.get_current_group());
        auto groups_by_body = doc.get_groups_by_body();

        UUID group_after;
        using Op = WorkspaceBrowser::MoveGroup;
        switch (op) {
        case Op::UP:
            group_after = doc.get_group_rel(group.m_uuid, -2);
            break;
        case Op::DOWN:
            group_after = doc.get_group_rel(group.m_uuid, 1);
            break;
        case Op::END_OF_DOCUMENT:
            group_after = groups_by_body.back().groups.back()->m_uuid;
            break;
        case Op::END_OF_BODY: {
            for (auto it_body = groups_by_body.begin(); it_body != groups_by_body.end(); it_body++) {
                auto it_group = std::ranges::find(it_body->groups, &group);
                if (it_group == it_body->groups.end())
                    continue;

                if (it_group == (it_body->groups.end() - 1)) {
                    // is at end of body, move to end of next body
                    auto it_next_body = it_body + 1;
                    if (it_next_body == groups_by_body.end()) {
                        it_next_body = it_body;
                    }
                    group_after = it_next_body->groups.back()->m_uuid;
                }
                else {
                    group_after = it_body->groups.back()->m_uuid;
                }
            }

        } break;
        }
        if (!group_after)
            return;

        std::cout << "move after " << doc.get_group(group_after).m_name << std::endl;

        if (!doc.reorder_group(group.m_uuid, group_after))
            return;
        m_core.set_needs_save();
        m_core.rebuild("reorder_group");
        canvas_update_keep_selection();
        m_workspace_browser->update_documents(m_document_view);
    });


    m_core.signal_rebuilt().connect([this] { m_workspace_browser->update_documents(m_document_view); });
    update_group_editor();
    m_core.signal_rebuilt().connect([this] {
        if (m_group_editor) {
            if (m_core.get_current_group() != m_group_editor->get_group())
                update_group_editor();
            else
                m_group_editor->reload();
        }
    });

    m_workspace_browser->signal_group_checked().connect([this](const UUID &uu_doc, const UUID &uu_group, bool checked) {
        m_document_view.m_group_views[uu_group].m_visible = checked;
        m_workspace_browser->update_current_group(m_document_view);
        canvas_update_keep_selection();
    });
    m_workspace_browser->signal_body_checked().connect([this](const UUID &uu_doc, const UUID &uu_group, bool checked) {
        m_document_view.m_body_views[uu_group].m_visible = checked;
        m_workspace_browser->update_current_group(m_document_view);
        canvas_update_keep_selection();
    });
    m_workspace_browser->signal_body_solid_model_checked().connect(
            [this](const UUID &uu_doc, const UUID &uu_group, bool checked) {
                m_document_view.m_body_views[uu_group].m_solid_model_visible = checked;
                m_workspace_browser->update_current_group(m_document_view);
                canvas_update_keep_selection();
            });

    m_core.signal_documents_changed().connect([this] {
        canvas_update_keep_selection();
        m_workspace_browser->update_documents(m_document_view);
        update_group_editor();
        update_workplane_label();
        update_action_sensitivity();
        m_workspace_browser->set_sensitive(m_core.has_documents());
        update_version_info();
    });
    m_workspace_browser->set_sensitive(m_core.has_documents());

    {
        Gtk::Box *lollipop_box = refBuilder->get_widget<Gtk::Box>("lollipop_box");
        auto axes_lollipop = Gtk::make_managed<AxesLollipop>();
        lollipop_box->append(*axes_lollipop);
        m_canvas->signal_view_changed().connect(sigc::track_obj(
                [this, axes_lollipop] {
                    const float alpha = -glm::radians(m_canvas->get_cam_azimuth() + 90);
                    const float beta = glm::radians(m_canvas->get_cam_elevation() - 90);
                    axes_lollipop->set_angles(alpha, beta);
                },
                *axes_lollipop));
    }
    m_canvas->set_cam_azimuth(270);
    m_canvas->set_cam_elevation(80);

    connect_action(ActionID::VIEW_ALL, [this](auto &a) {
        m_canvas->set_cam_azimuth(270);
        m_canvas->set_cam_elevation(80);
        m_canvas->set_cam_distance(10);
        m_canvas->set_center({0, 0, 0});
    });

    m_version_info_bar = refBuilder->get_widget<Gtk::InfoBar>("version_info_bar");
    m_version_info_bar_label = refBuilder->get_widget<Gtk::Label>("version_info_bar_label");

    update_action_sensitivity();
    set_icon_name("dune3d");
}

void Dune3DAppWindow::show_save_dialog(std::function<void()> save_cb, std::function<void()> no_save_cb)
{
    auto dialog = Gtk::AlertDialog::create("Save changes before closing?");
    dialog->set_detail(
            "If you don't save, all your changes will be permanently "
            "lost.");
    dialog->set_buttons({"Cancel", "Close without saving", "Save"});
    dialog->set_cancel_button(0);
    dialog->set_default_button(0);
    dialog->choose(*this, [dialog, save_cb, no_save_cb](Glib::RefPtr<Gio::AsyncResult> &result) {
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

void Dune3DAppWindow::close_document(const UUID &doc_uu, std::function<void()> save_cb,
                                     std::function<void()> no_save_cb)
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

void Dune3DAppWindow::update_group_editor()
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
    m_group_editor_box->append(*m_group_editor);
}

void Dune3DAppWindow::update_workplane_label()
{
    if (!m_core.has_documents()) {
        m_workplane_label->set_text("No documents");
        return;
    }
    auto wrkpl_uu = m_core.get_current_workplane();
    if (!wrkpl_uu) {
        m_workplane_label->set_text("No Workplane");
    }
    else {
        auto &wrkpl = m_core.get_current_document().get_entity<EntityWorkplane>(wrkpl_uu);
        auto &wrkpl_group = m_core.get_current_document().get_group<Group>(wrkpl.m_group);
        m_workplane_label->set_text("Workplane in group " + wrkpl_group.m_name);
    }
}

KeyMatchResult Dune3DAppWindow::keys_match(const KeySequence &keys) const
{
    return key_sequence_match(m_keys_current, keys);
}

void Dune3DAppWindow::update_action_sensitivity()
{
    auto sel = m_canvas->get_selection();
    bool has_workplane = false;
    for (const auto &it : sel) {
        if (it.type == SelectableRef::Type::ENTITY) {
            if (m_core.get_current_document().m_entities.count(it.item)
                && m_core.get_current_document().m_entities.at(it.item)->get_type() == Entity::Type::WORKPLANE) {
                has_workplane = true;
                break;
            }
        }
    }
    m_action_sensitivity[ActionID::SET_WORKPLANE] = has_workplane;
    m_action_sensitivity[ActionID::UNSET_WORKPLANE] = m_core.has_documents();
    m_action_sensitivity[ActionID::UNDO] = m_core.can_undo();
    m_action_sensitivity[ActionID::REDO] = m_core.can_redo();
    m_action_sensitivity[ActionID::SAVE_ALL] = m_core.get_needs_save_any();
    m_action_sensitivity[ActionID::SAVE] = m_core.get_needs_save();
    m_action_sensitivity[ActionID::SAVE_AS] = m_core.has_documents();
    m_action_sensitivity[ActionID::CLOSE_DOCUMENT] = m_core.has_documents();
    m_action_sensitivity[ActionID::OPEN_DOCUMENT] = !m_core.has_documents();
    m_action_sensitivity[ActionID::EXPORT_SOLID_MODEL_STEP] = m_core.has_documents();
    m_action_sensitivity[ActionID::EXPORT_SOLID_MODEL_STL] = m_core.has_documents();
    m_action_sensitivity[ActionID::TOGGLE_SOLID_MODEL] = m_core.has_documents();
    m_action_sensitivity[ActionID::NEW_DOCUMENT] = !m_core.has_documents();

    if (m_core.has_documents()) {
        auto &current_group = m_core.get_current_document().get_group(m_core.get_current_group());
        auto groups_sorted = m_core.get_current_document().get_groups_sorted();
        assert(groups_sorted.size());
        const bool is_first = groups_sorted.front() == &current_group;
        const bool is_last = groups_sorted.back() == &current_group;
        m_action_sensitivity[ActionID::PREVIOUS_GROUP] = !is_first;
        m_action_sensitivity[ActionID::NEXT_GROUP] = !is_last;
    }
    else {
        m_action_sensitivity[ActionID::PREVIOUS_GROUP] = false;
        m_action_sensitivity[ActionID::NEXT_GROUP] = false;
    }

    m_signal_action_sensitive.emit();
}

void Dune3DAppWindow::apply_preferences()
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

    m_canvas->set_appearance(m_preferences.canvas.appearance);

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
        main_window->tool_bar_set_vertical(preferences.tool_bar.vertical_layout);
        m_core.set_history_max(preferences.undo_redo.max_depth);
        m_core.set_history_never_forgets(preferences.undo_redo.never_forgets);
        selection_history_manager.set_never_forgets(preferences.undo_redo.never_forgets);
        preferences_apply_appearance(preferences);
        */
}

Gtk::Button *Dune3DAppWindow::create_action_button(ActionToolID action)
{
    auto &catitem = action_catalog.at(action);
    auto button = Gtk::manage(new Gtk::Button(catitem.name));
    attach_action_button(*button, action);
    return button;
}

bool Dune3DAppWindow::trigger_action(ActionToolID action, ActionSource source)
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

bool Dune3DAppWindow::get_action_sensitive(ActionID action) const
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

void Dune3DAppWindow::attach_action_button(Gtk::Button &button, ActionToolID action)
{
    button.signal_clicked().connect([this, action] { trigger_action(action); });
    if (std::holds_alternative<ActionID>(action)) {
        m_signal_action_sensitive.connect(
                [this, &button, action] { button.set_sensitive(get_action_sensitive(std::get<ActionID>(action))); });
        button.set_sensitive(get_action_sensitive(std::get<ActionID>(action)));
    }
}

void Dune3DAppWindow::handle_tool_action(const ActionConnection &conn)
{
    tool_begin(std::get<ToolID>(conn.id));
}

ActionConnection &Dune3DAppWindow::connect_action(ToolID tool_id)
{
    return connect_action(tool_id, sigc::mem_fun(*this, &Dune3DAppWindow::handle_tool_action));
}

ActionConnection &
Dune3DAppWindow::connect_action_with_source(ActionToolID id,
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

ActionConnection &Dune3DAppWindow::connect_action(ActionToolID id, std::function<void(const ActionConnection &)> cb)
{
    return connect_action_with_source(id, [cb](const ActionConnection &conn, ActionSource) { cb(conn); });
}

void Dune3DAppWindow::canvas_update()
{
    auto docs = m_core.get_documents();
    auto hover_sel = m_canvas->get_hover_selection();
    m_canvas->clear();
    for (const auto doc : docs) {
        Renderer renderer(*m_canvas);
        renderer.m_solid_model_edge_select_mode = m_solid_model_edge_select_mode;
        renderer.render(doc->get_document(), doc->get_current_group(), m_document_view);
    }
    m_canvas->set_hover_selection(hover_sel);
    m_canvas->request_push();
}

void Dune3DAppWindow::canvas_update_keep_selection()
{
    auto sel = m_canvas->get_selection();
    canvas_update();
    m_canvas->set_selection(sel);
}

void Dune3DAppWindow::open_file_view(const Glib::RefPtr<Gio::File> &file)
{
    if (m_core.has_documents())
        return;
    std::cout << "open " << file->get_path() << std::endl;
    m_core.add_document(file->get_path());
}

void Dune3DAppWindow::tool_begin(ToolID id /*bool override_selection, const std::set<SelectableRef> &sel,
                         std::unique_ptr<ToolData> data*/)
{
    if (m_core.tool_is_active()) {
        Logger::log_critical("can't begin tool while tool is active", Logger::Domain::IMP);
        return;
    }

    ToolArgs args;
    // args.data = std::move(data);

    //    if (override_selection)
    //        args.selection = sel;
    //   else
    args.selection = m_canvas->get_selection();
    m_canvas->set_selection_mode(Canvas::SelectionMode::NONE);
    ToolResponse r = m_core.tool_begin(id, args);
    tool_process(r);
}


void Dune3DAppWindow::tool_update_data(std::unique_ptr<ToolData> data)
{
    if (m_core.tool_is_active()) {
        ToolArgs args;
        args.type = ToolEventType::DATA;
        args.data = std::move(data);
        ToolResponse r = m_core.tool_update(args);
        tool_process(r);
    }
}

void Dune3DAppWindow::enable_hover_selection()
{
    m_canvas->set_selection_mode(Canvas::SelectionMode::HOVER_ONLY);
}

std::optional<SelectableRef> Dune3DAppWindow::get_hover_selection() const
{
    return m_canvas->get_hover_selection();
}

void Dune3DAppWindow::tool_process(ToolResponse &resp)
{
    tool_process_one();
    while (auto args = m_core.get_pending_tool_args()) {
        m_core.tool_update(*args);

        tool_process_one();
    }
}

void Dune3DAppWindow::canvas_update_from_tool()
{
    canvas_update();
    m_canvas->set_selection(m_core.get_tool_selection());
}

void Dune3DAppWindow::tool_process_one()
{
    if (!m_core.tool_is_active()) {
        m_dialogs.close_nonmodal();
        // imp_interface->dialogs.close_nonmodal();
        // reset_tool_hint_label();
        // canvas->set_cursor_external(false);
        // canvas->snap_filter.clear();
        m_canvas->set_selection_mode(Canvas::SelectionMode::HOVER);
        m_no_canvas_update = false;
        m_solid_model_edge_select_mode = false;
    }
    if (!m_no_canvas_update)
        canvas_update();
    m_canvas->set_selection(m_core.get_tool_selection());
    /*  if (m_core.tool_is_active()) {
          canvas->set_selection(m_core.get_tool_selection());
      }
      else {
          canvas->set_selection(m_core.get_tool_selection(),
                                canvas->get_selection_mode() == CanvasGL::SelectionMode::NORMAL);
      }*/
}

glm::dvec3 Dune3DAppWindow::get_cursor_pos() const
{
    return m_canvas->get_cursor_pos();
}

glm::vec3 Dune3DAppWindow::get_cam_normal() const
{
    return m_canvas->get_cam_normal();
}

glm::dvec3 Dune3DAppWindow::get_cursor_pos_for_plane(glm::dvec3 origin, glm::dvec3 normal) const
{
    return m_canvas->get_cursor_pos_for_plane(origin, normal);
}

void Dune3DAppWindow::handle_cursor_move()
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
        auto pos = m_canvas->get_cursor_pos_win();
        auto delta = pos - m_cursor_pos_win_drag_begin;
        if (glm::length(delta) > 10) {
            ToolArgs args;
            args.selection = m_selection_for_drag;
            m_canvas->set_selection_mode(Canvas::SelectionMode::NONE);
            ToolResponse r = m_core.tool_begin(m_drag_tool, args, true);
            tool_process(r);

            m_selection_for_drag.clear();
            m_drag_tool = ToolID::NONE;
        }
    }
}

void Dune3DAppWindow::handle_click(unsigned int button, unsigned int n)
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
        auto sel = m_canvas->get_selection();
        if (sel.size() == 1) {
            if (auto action = get_doubleclick_action(*sel.begin())) {
                trigger_action(*action);
            }
        }
    }
    else if (button == 1) {
        auto hover_sel = m_canvas->get_hover_selection();
        if (!hover_sel)
            return;

        auto sel = m_canvas->get_selection();
        if (sel.contains(hover_sel.value())) {
            m_drag_tool = get_tool_for_drag_move(false, sel);
            if (m_drag_tool != ToolID::NONE) {
                // canvas->inhibit_drag_selection();
                m_cursor_pos_win_drag_begin = m_canvas->get_cursor_pos_win();
                // cursor_pos_grid_drag_begin = m_canvas->get_cursor_pos();
                m_selection_for_drag = sel;
            }
        }
    }
}

std::optional<ActionToolID> Dune3DAppWindow::get_doubleclick_action(const SelectableRef &sr)
{
    if (sr.type == SelectableRef::Type::CONSTRAINT) {
        auto &constraint = m_core.get_current_document().get_constraint(sr.item);
        switch (constraint.get_type()) {
        case Constraint::Type::DIAMETER:
        case Constraint::Type::RADIUS:
        case Constraint::Type::POINT_DISTANCE:
        case Constraint::Type::POINT_DISTANCE_HORIZONTAL:
        case Constraint::Type::POINT_DISTANCE_VERTICAL:
            return ToolID::ENTER_DATUM;
            break;
        default:;
        }
    }
    return {};
}

ToolID Dune3DAppWindow::get_tool_for_drag_move(bool ctrl, const std::set<SelectableRef> &sel)
{
    return ToolID::MOVE;
}

void Dune3DAppWindow::reset_key_hint_label()
{
    const auto act = ActionID::POPOVER;
    if (m_action_connections.count(act)) {
        if (m_action_connections.at(act).key_sequences.size()) {
            const auto keys = key_sequence_to_string(m_action_connections.at(act).key_sequences.front());
            m_key_hint_label->set_text("> " + keys + " for menu");
            return;
        }
    }
    m_key_hint_label->set_text(">");
}


bool Dune3DAppWindow::handle_action_key(unsigned int keyval, Gdk::ModifierType state)
{
    if (keyval == GDK_KEY_Escape) {
        if (!m_core.tool_is_active()) {
            m_canvas->set_selection_mode(Canvas::SelectionMode::HOVER);
            m_canvas->set_selection({});

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
        m_keys_current.push_back({keyval, state
                                                  & (Gdk::ModifierType::SHIFT_MASK | Gdk::ModifierType::CONTROL_MASK
                                                     | Gdk::ModifierType::ALT_MASK)});
        auto in_tool_actions = m_core.get_tool_actions();
        std::map<InToolActionID, std::pair<KeyMatchResult, KeySequence>> in_tool_actions_matched;
        std::map<ActionConnection *, std::pair<KeyMatchResult, KeySequence>> connections_matched;
        auto selection = m_canvas->get_selection();
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
            m_key_hint_label->set_text("Ambiguous");
            m_keys_current.clear();
        }
        else if (connections_matched.size() == 1) {
            m_key_hint_label->set_text(key_sequence_to_string(m_keys_current));
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
            m_key_hint_label->set_text(key_sequence_to_string(m_keys_current));
            m_keys_current.clear();

            ToolArgs args;
            args.type = ToolEventType::ACTION;
            args.action = in_tool_actions_matched.begin()->first;
            ToolResponse r = m_core.tool_update(args);
            tool_process(r);

            return true;
        }
        else if (connections_matched.size() > 1 || in_tool_actions_matched.size() > 1) { // still ambigous
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
                 m_key_hint_label->set_text("Key sequences conflict");
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
            m_key_hint_label->set_text(key_sequence_to_string(m_keys_current) + "?");
            return true;
        }
        else if (connections_matched.size() == 0 || in_tool_actions_matched.size() == 0) {
            m_key_hint_label->set_text("Unknown key sequence");
            m_keys_current.clear();
            // main_window->key_hint_set_visible(false);
            return false;
        }
        else {
            Logger::log_warning("Key sequence??", Logger::Domain::IMP,
                                std::to_string(connections_matched.size()) + " "
                                        + std::to_string(in_tool_actions_matched.size()));
            return false;
        }
    }
    return false;
}


void Dune3DAppWindow::tool_bar_set_visible(bool v)
{
    if (v == false && m_tip_timeout_connection) { // hide and tip is still shown
        m_tool_bar_queue_close = true;
    }
    else {
        m_tool_bar->set_reveal_child(v);
        if (v) {
            m_tool_bar_queue_close = false;
        }
    }
}

void Dune3DAppWindow::tool_bar_set_tool_name(const std::string &s)
{
    m_tool_bar_name_label->set_text(s);
}

void Dune3DAppWindow::tool_bar_set_tool_tip(const std::string &s)
{
    if (s.size()) {
        m_tool_bar_tip_label->set_markup(s);
        m_tool_bar_tip_label->show();
    }
    else {
        m_tool_bar_tip_label->hide();
    }
}

void Dune3DAppWindow::tool_bar_flash(const std::string &s)
{
    tool_bar_flash(s, false);
}

void Dune3DAppWindow::tool_bar_flash_replace(const std::string &s)
{
    tool_bar_flash(s, true);
}

void Dune3DAppWindow::tool_bar_flash(const std::string &s, bool replace)
{
    if (m_flash_text.size() && !replace)
        m_flash_text += "; " + s;
    else
        m_flash_text = s;

    m_tool_bar_flash_label->set_markup(m_flash_text);

    m_tool_bar_stack->set_visible_child(*m_tool_bar_flash_label);
    m_tip_timeout_connection.disconnect();
    m_tip_timeout_connection = Glib::signal_timeout().connect(
            [this] {
                m_tool_bar_stack->set_visible_child(*m_tool_bar_box);

                m_flash_text.clear();
                if (m_tool_bar_queue_close)
                    m_tool_bar->set_reveal_child(false);
                m_tool_bar_queue_close = false;
                return false;
            },
            2000);
}

void Dune3DAppWindow::tool_bar_set_vertical(bool v)
{
    m_tool_bar_box->set_orientation(v ? Gtk::Orientation::VERTICAL : Gtk::Orientation::HORIZONTAL);
}

void Dune3DAppWindow::handle_tool_change()
{
    const auto tool_id = m_core.get_tool_id();
    // panels->set_sensitive(id == ToolID::NONE);
    // canvas->set_selection_allowed(id == ToolID::NONE);
    // main_window->tool_bar_set_use_actions(core->get_tool_actions().size());
    if (tool_id != ToolID::NONE) {
        tool_bar_set_tool_name(action_catalog.at(tool_id).name);
        tool_bar_set_tool_tip("");
    }
    tool_bar_set_visible(tool_id != ToolID::NONE);
    tool_bar_clear_actions();
}

void Dune3DAppWindow::tool_bar_clear_actions()
{
    for (auto w : m_action_widgets) {
        m_tool_bar_actions_box->remove(*w);
    }
    m_action_widgets.clear();
    m_in_tool_action_label_infos.clear();

    // for (auto ch : m_tool_bar_actions_box->get_first_child())
    //    delete ch;
}

void Dune3DAppWindow::tool_bar_append_action(Gtk::Widget &w)
{
    m_tool_bar_actions_box->append(w);
    m_action_widgets.push_back(&w);
    m_tool_bar_actions_box->show();
}


void Dune3DAppWindow::tool_bar_set_actions(const std::vector<ActionLabelInfo> &labels)
{
    if (m_in_tool_action_label_infos != labels) {
        tool_bar_clear_actions();
        for (const auto &it : labels) {
            tool_bar_append_action(it.action1, it.action2, it.label);
        }

        m_in_tool_action_label_infos = labels;
    }
}

void Dune3DAppWindow::tool_bar_append_action(InToolActionID action1, InToolActionID action2, const std::string &s)
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
        key_box->get_style_context()->add_class("imp-key-box");

        box->append(*key_box);
    }
    const auto &as = s.size() ? s : in_tool_action_catalog.at(action1).name;

    auto la = Gtk::manage(new Gtk::Label(as));
    la->set_valign(Gtk::Align::BASELINE);

    la->show();

    box->append(*la);

    tool_bar_append_action(*box);
}

void Dune3DAppWindow::set_version_info(const std::string &s)
{
    if (s.size()) {
        m_version_info_bar->set_revealed(true);
        m_version_info_bar_label->set_markup(s);
    }
    else {
        m_version_info_bar->set_revealed(false);
    }
}

void Dune3DAppWindow::update_version_info()
{
    if (!m_core.has_documents()) {
        set_version_info("");
        return;
    }
    const auto &doc = m_core.get_current_document();
    auto &ver = doc.m_version;
    set_version_info(ver.get_message());
}

} // namespace dune3d

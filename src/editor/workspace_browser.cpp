#include "workspace_browser.hpp"
#include "core/core.hpp"
#include "document/document.hpp"
#include "document/group/group.hpp"
#include "document/group/group_sweep.hpp"
#include "document_view.hpp"
#include <iostream>

namespace dune3d {

class WorkspaceBrowser::GroupItem : public Glib::Object {
public:
    static Glib::RefPtr<GroupItem> create()
    {
        return Glib::make_refptr_for_instance<GroupItem>(new GroupItem);
    }


    Glib::Property<Glib::ustring> m_name;
    Glib::Property<bool> m_active;
    Glib::Property<bool> m_check_active;
    Glib::Property<bool> m_check_sensitive;
    Glib::Property<int> m_dof;
    Glib::Property<GroupStatusMessage::Status> m_status;
    Glib::Property<Glib::ustring> m_status_message;
    Glib::Property<bool> m_source_group;
    UUID m_uuid;
    UUID m_doc;

    // No idea why the ObjectBase::get_type won't work for us but
    // reintroducing the method and using the name used by gtkmm seems
    // to work.
    static GType get_type()
    {
        // Let's cache once the type does exist.
        if (!gtype)
            gtype = g_type_from_name("gtkmm__CustomObject_GroupItem");
        return gtype;
    }

private:
    GroupItem()
        : Glib::ObjectBase("GroupItem"), m_name(*this, "name"), m_active(*this, "active", false),
          m_check_active(*this, "check_active", false), m_check_sensitive(*this, "check_sensitive", true),
          m_dof(*this, "dof"), m_status(*this, "status", GroupStatusMessage::Status::NONE),
          m_status_message(*this, "status_message"), m_source_group(*this, "source_group")
    {
    }

    static GType gtype;
};

GType WorkspaceBrowser::GroupItem::gtype;


class WorkspaceBrowser::BodyItem : public Glib::Object {
public:
    static Glib::RefPtr<BodyItem> create()
    {
        return Glib::make_refptr_for_instance<BodyItem>(new BodyItem);
    }


    Glib::Property<Glib::ustring> m_name;
    UUID m_uuid;
    UUID m_doc;
    Glib::Property<bool> m_check_active;
    Glib::Property<bool> m_check_sensitive;
    Glib::Property<bool> m_solid_model_active;
    Glib::RefPtr<Gio::ListStore<GroupItem>> m_group_store;

    // No idea why the ObjectBase::get_type won't work for us but
    // reintroducing the method and using the name used by gtkmm seems
    // to work.
    static GType get_type()
    {
        // Let's cache once the type does exist.
        if (!gtype)
            gtype = g_type_from_name("gtkmm__CustomObject_BodyItem");
        return gtype;
    }

private:
    BodyItem()
        : Glib::ObjectBase("BodyItem"), m_name(*this, "name"), m_check_active(*this, "check_active", false),
          m_check_sensitive(*this, "check_sensitive", true), m_solid_model_active(*this, "m_solid_model_active", true)
    {
        m_group_store = Gio::ListStore<GroupItem>::create();
    }

    static GType gtype;
};

GType WorkspaceBrowser::BodyItem::gtype;

class WorkspaceBrowser::DocumentItem : public Glib::Object {
public:
    static Glib::RefPtr<DocumentItem> create()
    {
        return Glib::make_refptr_for_instance<DocumentItem>(new DocumentItem);
    }


    Glib::Property<Glib::ustring> m_name;
    UUID m_uuid;
    Glib::RefPtr<Gio::ListStore<BodyItem>> m_body_store;

    // No idea why the ObjectBase::get_type won't work for us but
    // reintroducing the method and using the name used by gtkmm seems
    // to work.
    static GType get_type()
    {
        // Let's cache once the type does exist.
        if (!gtype)
            gtype = g_type_from_name("gtkmm__CustomObject_DocumentItem");
        return gtype;
    }

private:
    DocumentItem() : Glib::ObjectBase("DocumentItem"), m_name(*this, "name")
    {
        m_body_store = Gio::ListStore<BodyItem>::create();
    }

    static GType gtype;
};

GType WorkspaceBrowser::DocumentItem::gtype;

void WorkspaceBrowser::update_documents(const DocumentView &doc_view)
{
    block_signals();
    auto store = Gio::ListStore<DocumentItem>::create();
    for (auto doci : m_core.get_documents()) {
        auto mi = DocumentItem::create();
        mi->m_name = doci->get_basename();
        mi->m_uuid = doci->get_uuid();
        Glib::RefPtr<BodyItem> body_item = BodyItem::create();
        body_item->m_name = "Missing";
        for (auto gr : doci->get_document().get_groups_sorted()) {
            if (gr->m_body.has_value()) {
                body_item = BodyItem::create();
                body_item->m_name = gr->m_body->m_name;
                body_item->m_uuid = gr->m_uuid;
                mi->m_body_store->append(body_item);
            }

            auto gi = GroupItem::create();
            gi->m_name = gr->m_name;
            gi->m_uuid = gr->m_uuid;
            gi->m_doc = doci->get_uuid();
            body_item->m_group_store->append(gi);
        }
        store->append(mi);
    }
    m_document_store = store;
    m_model = Gtk::TreeListModel::create(m_document_store, sigc::mem_fun(*this, &WorkspaceBrowser::create_model),
                                         /* passthrough */ false, /* autoexpand */ true);
    m_selection_model->set_model(m_model);
    unblock_signals();
    update_current_group(doc_view);
    // m_selection_model->set_selected(sel);
}

void WorkspaceBrowser::block_signals()
{
    m_signal_group_checked.block();
    m_signal_body_checked.block();
    m_signal_group_selected.block();
    m_signal_body_solid_model_checked.block();
}

void WorkspaceBrowser::unblock_signals()
{
    m_signal_group_checked.unblock();
    m_signal_body_checked.unblock();
    m_signal_group_selected.unblock();
    m_signal_body_solid_model_checked.unblock();
}

void WorkspaceBrowser::update_current_group(const DocumentView &doc_view)
{
    block_signals();
    for (size_t i_doc = 0; i_doc < m_document_store->get_n_items(); i_doc++) {
        auto &it_doc = *m_document_store->get_item(i_doc);
        auto &doci = m_core.get_idocument_info(it_doc.m_uuid);
        it_doc.m_name = doci.get_basename();
        const auto &current_group = doci.get_document().get_group(doci.get_current_group());
        UUID source_group;
        if (auto group_sweep = dynamic_cast<const GroupSweep *>(&current_group))
            source_group = group_sweep->m_source_group;
        auto body = current_group.find_body(doci.get_document());
        UUID body_uu = body.group.m_uuid;
        bool after_active = false;
        for (size_t i_body = 0; i_body < it_doc.m_body_store->get_n_items(); i_body++) {
            auto &it_body = *it_doc.m_body_store->get_item(i_body);

            it_body.m_check_sensitive = body_uu != it_body.m_uuid;
            if (body_uu == it_body.m_uuid)
                it_body.m_check_active = true;
            else
                it_body.m_check_active = doc_view.body_is_visible(it_body.m_uuid);

            it_body.m_solid_model_active = doc_view.body_solid_model_is_visible(it_body.m_uuid);

            for (size_t i_group = 0; i_group < it_body.m_group_store->get_n_items(); i_group++) {
                auto &it_group = *it_body.m_group_store->get_item(i_group);
                bool is_current = doci.get_current_group() == it_group.m_uuid;
                it_group.m_active = is_current;
                auto &gr = doci.get_document().get_group(it_group.m_uuid);
                it_group.m_dof = gr.m_dof;
                it_group.m_name = gr.m_name;
                it_group.m_source_group = it_group.m_uuid == source_group;
                if (is_current) {
                    it_group.m_check_active = true;
                    it_group.m_check_sensitive = false;
                }
                else if (after_active) {
                    it_group.m_check_active = false;
                    it_group.m_check_sensitive = false;
                }
                else {
                    it_group.m_check_sensitive = true;
                    it_group.m_check_active = doc_view.group_is_visible(it_group.m_uuid);
                }
                {
                    auto msgs = gr.get_messages();
                    it_group.m_status = GroupStatusMessage::summarize(msgs);
                    Glib::ustring txt;
                    for (auto &msg : msgs) {
                        txt += msg.message + "\n";
                    }
                    it_group.m_status_message = txt;
                }
                if (is_current)
                    after_active = true;
            }
        }
        select_group(doci.get_current_group());
    }

    unblock_signals();
}

class SolidModelToggleButton : public Gtk::ToggleButton {
public:
    SolidModelToggleButton()
    {
        add_css_class("solid-model-toggle-button");
        set_has_frame(false);
        update_icon();
        signal_toggled().connect(sigc::mem_fun(*this, &SolidModelToggleButton::update_icon));
    }

private:
    void update_icon()
    {
        if (get_active()) {
            set_image_from_icon_name("view-show-solid-model-symbolic");
        }
        else {
            set_image_from_icon_name("view-hide-solid-model-symbolic");
        }
    }
};

class WorkspaceBrowser::WorkspaceRow : public Gtk::TreeExpander {
public:
    WorkspaceRow(WorkspaceBrowser &browser) : m_browser(browser)
    {
        m_checkbutton = Gtk::make_managed<Gtk::CheckButton>();
        m_checkbutton->set_active(true);

        m_solid_toggle = Gtk::make_managed<SolidModelToggleButton>();
        m_solid_toggle->signal_toggled().connect([this] {
            m_browser.signal_body_solid_model_checked().emit(m_body->m_doc, m_body->m_uuid,
                                                             m_solid_toggle->get_active());
        });

        m_label = Gtk::make_managed<Gtk::Label>();
        m_label->set_halign(Gtk::Align::START);

        m_source_group_image = Gtk::make_managed<Gtk::Image>();
        m_source_group_image->set_hexpand(true);
        m_source_group_image->set_from_icon_name("action-link-symbolic");
        m_source_group_image->set_halign(Gtk::Align::START);
        m_source_group_image->set_tooltip_text("Source of current group");

        m_status_button = Gtk::make_managed<Gtk::MenuButton>();
        m_status_button->set_icon_name("dialog-information-symbolic");
        m_status_button->set_has_frame(false);

        m_status_label = Gtk::make_managed<Gtk::Label>();
        m_status_label->set_hexpand(true);
        m_status_label->set_halign(Gtk::Align::START);

        m_close_button = Gtk::make_managed<Gtk::Button>();
        m_close_button->set_icon_name("window-close-symbolic");
        m_close_button->set_has_frame(false);
        m_close_button->signal_clicked().connect([this] { m_browser.m_signal_close_document.emit(m_doc->m_uuid); });

        {
            auto popover = Gtk::make_managed<Gtk::Popover>();
            popover->set_child(*m_status_label);
            m_status_button->set_popover(*popover);
        }


        {
            auto attr = Pango::Attribute::create_attr_weight(Pango::Weight::BOLD);
            m_attrs_bold.insert(attr);
        }

        m_checkbutton->signal_toggled().connect([this] {
            if (m_body)
                m_browser.signal_body_checked().emit(m_body->m_doc, m_body->m_uuid, m_checkbutton->get_active());
            if (m_group)
                m_browser.signal_group_checked().emit(m_group->m_doc, m_group->m_uuid, m_checkbutton->get_active());
        });

        m_dof_label = Gtk::make_managed<Gtk::Label>("0");
        m_dof_label->add_css_class("dim-label");
        m_dof_label->set_width_chars(2);

        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 5);
        auto box2 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 5);
        box2->set_hexpand(true);
        box->append(*m_checkbutton);
        box->append(*m_solid_toggle);
        box2->append(*m_label);
        box2->append(*m_source_group_image);
        box->append(*box2);

        box->append(*m_status_button);
        box->append(*m_close_button);
        box->append(*m_dof_label);

        set_child(*box);
    }

    void bind(DocumentItem &it)
    {
        m_doc = &it;
        m_browser.block_signals();
        m_checkbutton->set_visible(false);
        m_solid_toggle->set_visible(false);
        m_dof_label->set_visible(false);
        m_status_button->set_visible(false);
        m_close_button->set_visible(true);
        m_source_group_image->set_visible(false);
        m_label->set_attributes(m_attrs_normal);
        m_bindings.push_back(Glib::Binding::bind_property_value(it.m_name.get_proxy(), m_label->property_label(),
                                                                Glib::Binding::Flags::SYNC_CREATE));
        m_browser.unblock_signals();
    }
    void bind(BodyItem &it)
    {
        m_browser.block_signals();
        m_body = &it;
        m_checkbutton->set_active(true);
        m_checkbutton->set_sensitive(true);
        m_solid_toggle->set_visible(true);
        m_dof_label->set_visible(false);
        m_status_button->set_visible(false);
        m_close_button->set_visible(false);
        m_source_group_image->set_visible(false);
        m_label->set_attributes(m_attrs_normal);
        m_bindings.push_back(Glib::Binding::bind_property_value(it.m_name.get_proxy(), m_label->property_label(),
                                                                Glib::Binding::Flags::SYNC_CREATE));
        m_bindings.push_back(Glib::Binding::bind_property_value(
                it.m_check_active.get_proxy(), m_checkbutton->property_active(), Glib::Binding::Flags::SYNC_CREATE));
        m_bindings.push_back(Glib::Binding::bind_property_value(it.m_check_sensitive.get_proxy(),
                                                                m_checkbutton->property_sensitive(),
                                                                Glib::Binding::Flags::SYNC_CREATE));

        m_bindings.push_back(Glib::Binding::bind_property_value(it.m_solid_model_active.get_proxy(),
                                                                m_solid_toggle->property_active(),
                                                                Glib::Binding::Flags::SYNC_CREATE));
        m_browser.unblock_signals();
    }
    void bind(GroupItem &it)
    {
        m_browser.block_signals();
        m_solid_toggle->set_visible(false);
        m_dof_label->set_visible(true);
        m_status_button->set_visible(true);
        m_close_button->set_visible(false);
        m_group = &it;
        m_bindings.push_back(Glib::Binding::bind_property_value(it.m_name.get_proxy(), m_label->property_label(),
                                                                Glib::Binding::Flags::SYNC_CREATE));
        m_bindings.push_back(Glib::Binding::bind_property_value(
                it.m_status_message.get_proxy(), m_status_label->property_label(), Glib::Binding::Flags::SYNC_CREATE));
        update_label_attrs(it);
        m_connections.push_back(
                it.m_active.get_proxy().signal_changed().connect([this, &it] { update_label_attrs(it); }));

        m_bindings.push_back(Glib::Binding::bind_property_value(
                it.m_check_active.get_proxy(), m_checkbutton->property_active(), Glib::Binding::Flags::SYNC_CREATE));

        m_bindings.push_back(Glib::Binding::bind_property_value(it.m_check_sensitive.get_proxy(),
                                                                m_checkbutton->property_sensitive(),
                                                                Glib::Binding::Flags::SYNC_CREATE));
        m_bindings.push_back(Glib::Binding::bind_property_value(it.m_source_group.get_proxy(),
                                                                m_source_group_image->property_visible(),
                                                                Glib::Binding::Flags::SYNC_CREATE));
        m_dof_label->set_text(std::to_string(it.m_dof.get_value()));
        m_connections.push_back(it.m_dof.get_proxy().signal_changed().connect(
                [this, &it] { m_dof_label->set_text(std::to_string(it.m_dof.get_value())); }));

        set_status(it.m_status.get_value());
        m_connections.push_back(
                it.m_status.get_proxy().signal_changed().connect([this, &it] { set_status(it.m_status.get_value()); }));

        m_browser.unblock_signals();
    }

    void unbind()
    {
        for (auto &conn : m_connections)
            conn.disconnect();
        m_connections.clear();
        for (auto bind : m_bindings)
            bind->unbind();
        m_bindings.clear();
        m_group = nullptr;
        m_body = nullptr;
        m_doc = nullptr;
    }

private:
    WorkspaceBrowser &m_browser;

    const DocumentItem *m_doc = nullptr;
    const GroupItem *m_group = nullptr;
    const BodyItem *m_body = nullptr;

    Gtk::CheckButton *m_checkbutton = nullptr;
    Gtk::ToggleButton *m_solid_toggle = nullptr;
    Gtk::Label *m_label = nullptr;
    Gtk::Image *m_source_group_image = nullptr;
    Gtk::Label *m_dof_label = nullptr;
    Gtk::MenuButton *m_status_button = nullptr;
    Gtk::Label *m_status_label = nullptr;
    Gtk::Button *m_close_button = nullptr;
    std::vector<Glib::RefPtr<Glib::Binding>> m_bindings;
    std::vector<sigc::connection> m_connections;

    Pango::AttrList m_attrs_normal;
    Pango::AttrList m_attrs_bold;

    void update_label_attrs(GroupItem &it)
    {
        if (it.m_active.get_value())
            m_label->set_attributes(m_attrs_bold);
        else
            m_label->set_attributes(m_attrs_normal);
    }

    void set_status(GroupStatusMessage::Status st)
    {
        using S = GroupStatusMessage::Status;
        m_status_button->set_visible(st != S::NONE);
        switch (st) {
        case S::NONE:
            break;
        case S::INFO:
            m_status_button->set_icon_name("dialog-information-symbolic");
            break;
        case S::WARN:
            m_status_button->set_icon_name("dialog-warning-symbolic");
            break;
        case S::ERR:
            m_status_button->set_icon_name("dialog-error-symbolic");
            break;
        }
    }
};

WorkspaceBrowser::WorkspaceBrowser(Core &core) : Gtk::Box(Gtk::Orientation::VERTICAL), m_core(core)
{
    m_document_store = Gio::ListStore<DocumentItem>::create();


    // Set list model and selection model.
    // passthrough must be false when Gtk::TreeExpander is used in the view.
    m_model = Gtk::TreeListModel::create(m_document_store, sigc::mem_fun(*this, &WorkspaceBrowser::create_model),
                                         /* passthrough */ false, /* autoexpand */ true);
    m_selection_model = Gtk::SingleSelection::create(m_model);

    auto factory = Gtk::SignalListItemFactory::create();
    factory->signal_setup().connect([this](const Glib::RefPtr<Gtk::ListItem> &list_item) {
        // Each ListItem contains a TreeExpander, which contains a Label.
        // The Label shows the StringObject's string. That's done in on_bind_name().
        auto expander = Gtk::make_managed<WorkspaceRow>(*this);
        list_item->set_child(*expander);
    });
    factory->signal_bind().connect([this](const Glib::RefPtr<Gtk::ListItem> &list_item) {
        // When TreeListModel::property_passthrough() is false, ListItem::get_item()
        // is a TreeListRow. TreeExpander needs the TreeListRow.
        // The StringObject item is returned by TreeListRow::get_item().
        auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(list_item->get_item());
        if (!row)
            return;
        auto expander = dynamic_cast<WorkspaceRow *>(list_item->get_child());
        if (!expander)
            return;
        expander->set_list_row(row);
        if (auto col = std::dynamic_pointer_cast<DocumentItem>(row->get_item())) {
            expander->bind(*col);
            // expander->set_label(col->m_name.get_value());
        }
        else if (auto col = std::dynamic_pointer_cast<BodyItem>(row->get_item())) {
            expander->bind(*col);
            // expander->set_label("Body " + col->m_name.get_value());
        }
        else if (auto col = std::dynamic_pointer_cast<GroupItem>(row->get_item())) {
            expander->bind(*col);
            // expander->set_label("Group " + col->m_name.get_value());
        }
    });
    factory->signal_unbind().connect([this](const Glib::RefPtr<Gtk::ListItem> &list_item) {
        // When TreeListModel::property_passthrough() is false, ListItem::get_item()
        // is a TreeListRow. TreeExpander needs the TreeListRow.
        // The StringObject item is returned by TreeListRow::get_item().
        auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(list_item->get_item());
        if (!row)
            return;
        auto expander = dynamic_cast<WorkspaceRow *>(list_item->get_child());
        if (!expander)
            return;
        expander->unbind();
    });


    m_view = Gtk::make_managed<Gtk::ListView>(m_selection_model, factory);
    // m_view->set_single_click_activate(true);
    /*  m_view->signal_activate().connect([this](guint index) {
          auto it = m_model->get_row(index);
          auto tr = std::dynamic_pointer_cast<Gtk::TreeListRow>(it);
          if (!tr)
              return;
          if (auto gr = std::dynamic_pointer_cast<WorkspaceBrowser::GroupItem>(tr->get_item())) {
              std::cout << "sel gr " << gr->m_name << std::endl;
              m_signal_group_selected.emit(gr->m_doc, gr->m_uuid);
          }
      });*/
    m_selection_model->signal_selection_changed().connect([this](guint, guint) {
        auto sel = m_selection_model->get_selected_item();
        auto tr = std::dynamic_pointer_cast<Gtk::TreeListRow>(sel);
        if (!tr)
            return;
        if (auto gr = std::dynamic_pointer_cast<WorkspaceBrowser::GroupItem>(tr->get_item())) {
            m_signal_group_selected.emit(gr->m_doc, gr->m_uuid);
        }
    });
    m_view->add_css_class("navigation-sidebar");
    m_sc = Gtk::make_managed<Gtk::ScrolledWindow>();
    m_sc->set_child(*m_view);
    m_sc->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    m_sc->set_vexpand(true);
    append(*m_sc);

    {
        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
        box->add_css_class("toolbar");
        {
            auto button = Gtk::make_managed<Gtk::MenuButton>();
            {
                auto top = Gio::Menu::create();
                auto actions = Gio::SimpleActionGroup::create();
                actions->add_action("sketch", [this] { m_signal_add_group.emit(Group::Type::SKETCH); });
                actions->add_action("extrude", [this] { m_signal_add_group.emit(Group::Type::EXTRUDE); });
                actions->add_action("lathe", [this] { m_signal_add_group.emit(Group::Type::LATHE); });
                actions->add_action("fillet", [this] { m_signal_add_group.emit(Group::Type::FILLET); });
                actions->add_action("chamfer", [this] { m_signal_add_group.emit(Group::Type::CHAMFER); });
                actions->add_action("linear_array", [this] { m_signal_add_group.emit(Group::Type::LINEAR_ARRAY); });
                top->append_item(Gio::MenuItem::create("Sketch", "groups.sketch"));
                top->append_item(Gio::MenuItem::create("Extrude", "groups.extrude"));
                top->append_item(Gio::MenuItem::create("Lathe", "groups.lathe"));
                top->append_item(Gio::MenuItem::create("Linear array", "groups.linear_array"));
                top->append_item(Gio::MenuItem::create("Fillet", "groups.fillet"));
                top->append_item(Gio::MenuItem::create("Chamfer", "groups.chamfer"));


                insert_action_group("groups", actions);
                button->set_menu_model(top);
            }
            button->set_icon_name("list-add-symbolic");
            button->set_direction(Gtk::ArrowType::UP);
            box->append(*button);
        }
        {
            auto button = Gtk::make_managed<Gtk::Button>();
            button->set_icon_name("list-remove-symbolic");
            button->signal_clicked().connect([this] { m_signal_delete_current_group.emit(); });
            box->append(*button);
        }
        {
            auto button = Gtk::make_managed<Gtk::Button>();
            button->set_icon_name("go-up-symbolic");
            button->set_tooltip_text("Move group up");
            button->signal_clicked().connect([this] { m_signal_move_group.emit(Document::MoveGroup::UP); });
            box->append(*button);
        }
        {
            auto button = Gtk::make_managed<Gtk::Button>();
            button->set_icon_name("go-down-symbolic");
            button->set_tooltip_text("Move group down");
            button->signal_clicked().connect([this] { m_signal_move_group.emit(Document::MoveGroup::DOWN); });
            box->append(*button);
        }
        {
            auto button = Gtk::make_managed<Gtk::Button>();
            button->set_icon_name("action-move-group-down2-symbolic");
            button->set_tooltip_text("Move group to end of body / next body");
            button->signal_clicked().connect([this] { m_signal_move_group.emit(Document::MoveGroup::END_OF_BODY); });
            box->append(*button);
        }
        {
            auto button = Gtk::make_managed<Gtk::Button>();
            button->set_icon_name("action-move-group-down3-symbolic");
            button->set_tooltip_text("Move group to end of document");
            button->signal_clicked().connect(
                    [this] { m_signal_move_group.emit(Document::MoveGroup::END_OF_DOCUMENT); });
            box->append(*button);
        }
        append(*box);
    }
}

Glib::RefPtr<Gio::ListModel> WorkspaceBrowser::create_model(const Glib::RefPtr<Glib::ObjectBase> &item)
{
    // The items in a StringList are StringObjects.
    if (auto col = std::dynamic_pointer_cast<DocumentItem>(item))
        return col->m_body_store;
    if (auto col = std::dynamic_pointer_cast<BodyItem>(item))
        return col->m_group_store;
    return nullptr;
    /*Glib::RefPtr<Gio::ListModel> result;
    if (!col)
        // Top names
        result = Gtk::StringList::create({"Billy Bob", "Joey Jojo", "Rob McRoberts"});
    else if (col->get_string() == "Billy Bob")
        result = Gtk::StringList::create({"Billy Bob Junior", "Sue Bob"});
    else if (col->get_string() == "Rob McRoberts")
        result = Gtk::StringList::create({"Xavier McRoberts"});
*/
    // If result is empty, it's a leaf in the tree, i.e. an item without children.
    // Returning an empty RefPtr (not a RefPtr with an empty StringList)
    // signals that the item is not expandable.
    // return result;
}

void WorkspaceBrowser::group_prev_next(int dir)
{
    auto next_group = m_core.get_current_document().get_group_rel(m_core.get_current_group(), dir);
    if (!next_group)
        return;

    select_group(next_group);
}

void WorkspaceBrowser::select_group(const UUID &uu)
{
    const auto n = m_selection_model->get_n_items();

    for (size_t i = 0; i < n; i++) {
        auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(m_selection_model->get_object(i));
        if (!row)
            continue;
        auto it = std::dynamic_pointer_cast<GroupItem>(row->get_item());
        if (!it)
            continue;

        if (it->m_doc == m_core.get_current_idocument_info().get_uuid() && it->m_uuid == uu) {
            m_selection_model->select_item(i, true);
            return;
        }
    }
}

} // namespace dune3d

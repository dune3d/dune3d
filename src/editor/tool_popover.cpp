#include "tool_popover.hpp"
#include "action/action_catalog.hpp"
#include "util/str_util.hpp"
#include <iostream>

namespace dune3d {


class ToolPopover::ActionItem : public Glib::Object {
public:
    static Glib::RefPtr<ActionItem> create()
    {
        return Glib::make_refptr_for_instance<ActionItem>(new ActionItem);
    }

    ActionToolID m_id;
    std::string m_name;
    Glib::Property<Glib::ustring> m_keys;
    Glib::Property<bool> m_can_begin;

    // No idea why the ObjectBase::get_type won't work for us but
    // reintroducing the method and using the name used by gtkmm seems
    // to work.
    static GType get_type()
    {
        // Let's cache once the type does exist.
        if (!gtype)
            gtype = g_type_from_name("gtkmm__CustomObject_ActionItem");
        return gtype;
    }

private:
    ActionItem() : Glib::ObjectBase("ActionItem"), m_keys(*this, "keys"), m_can_begin(*this, "can_begin", true)
    {
    }

    static GType gtype;
};


GType ToolPopover::ActionItem::gtype;


class ToolPopover::GroupItem : public Glib::Object {
public:
    static Glib::RefPtr<GroupItem> create()
    {
        return Glib::make_refptr_for_instance<GroupItem>(new GroupItem);
    }

    ActionGroup m_group;
    std::string m_name;

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
    GroupItem() : Glib::ObjectBase("GroupItem")
    {
    }

    static GType gtype;
};

GType ToolPopover::GroupItem::gtype;


class ToolPopover::Filter : public Gtk::Filter {
public:
    static auto create()
    {
        return std::shared_ptr<Filter>(new Filter());
    }

    void set_search(std::string search)
    {
        if (search.size()) {
            std::transform(search.begin(), search.end(), search.begin(), tolower);
            search = "*" + search + "*";
            std::replace(search.begin(), search.end(), ' ', '*');

            m_pattern.reset(new Glib::PatternSpec(search));
        }
        else {
            m_pattern.reset();
        }
        changed();
    }

    void set_group(ActionGroup group)
    {
        m_selected_group = group;
        changed();
    }

    void refilter()
    {
        changed();
    }

    bool match_vfunc(const Glib::RefPtr<Glib::ObjectBase> &item) override
    {
        const auto it = std::dynamic_pointer_cast<ActionItem>(item);
        if (!it)
            return false;
        if (!it->m_can_begin)
            return false;

        if (m_selected_group != ActionGroup::ALL && !m_pattern) {
            // std::cout << static_cast<int>(action_catalog.at(it->m_id).group)
            //           << " != " << static_cast<int>(m_selected_group) << std::endl;
            if (action_catalog.at(it->m_id).group != m_selected_group)
                return false;
        }

        if (!m_pattern)
            return true;
        Glib::ustring tool_name_u = it->m_name;
        std::string tool_name(tool_name_u);
        std::transform(tool_name.begin(), tool_name.end(), tool_name.begin(), tolower);
        return m_pattern->match(tool_name);
    }

private:
    Filter()
    {
    }

    std::unique_ptr<Glib::PatternSpec> m_pattern;
    ActionGroup m_selected_group = ActionGroup::ALL;
};

ToolPopover::ToolPopover() : Gtk::Popover()
{
    set_has_arrow(false);
    // add_css_class("imp-tool-popover");
    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    search_entry = Gtk::manage(new Gtk::SearchEntry());
    search_entry->set_margin_bottom(5);
    search_entry->set_width_chars(45);
    box->append(*search_entry);

    search_entry->signal_changed().connect([this] {
        std::string search = search_entry->get_text();
        m_revealer->set_reveal_child(search.size() == 0);
        m_filter->set_search(search);
        if (m_selection->get_n_items() != 0)
            m_selection->set_selected(0);
    });
    search_entry->signal_stop_search().connect([this] { popdown(); });
    {
        auto controller = Gtk::EventControllerKey::create();
        controller->signal_key_pressed().connect(
                [this](guint keyval, guint keycode, Gdk::ModifierType state) -> bool {
                    if (auto sel = m_selection->get_selected(); sel != GTK_INVALID_LIST_POSITION) {
                        std::cout << "search sel " << keyval << std::endl;
                        if (keyval == GDK_KEY_Down) {
                            sel++;
                            if (sel >= m_selection->get_n_items())
                                return false;
                        }
                        else if (keyval == GDK_KEY_Up) {
                            if (sel == 0)
                                return false;
                            sel--;
                        }
                        else {
                            return false;
                        }
                        m_selection->select_item(sel, true);
                        m_view->grab_focus();
                    }


                    return false;
                },
                true);


        search_entry->add_controller(controller);
    }


    m_view = Gtk::make_managed<Gtk::ColumnView>();
    search_entry->set_key_capture_widget(*m_view);
    // m_view->set_single_click_activate(true);
    m_group_view = Gtk::make_managed<Gtk::ColumnView>();
    m_view->add_css_class("data-table");
    m_group_view->add_css_class("data-table");
    using ActionItemListModel = Gio::ListStore<ToolPopover::ActionItem>;
    using GroupItemListModel = Gio::ListStore<ToolPopover::GroupItem>;

    m_store = ActionItemListModel::create();
    m_group_store = GroupItemListModel::create();

    for (const auto &[id, it] : action_catalog) {
        if (!(it.flags & ActionCatalogItem::FLAGS_NO_POPOVER)) {
            auto mi = ActionItem::create();
            mi->m_id = id;
            mi->m_name = it.name;
            m_store->append(mi);
        }
    }


    {
        auto mi = GroupItem::create();
        mi->m_group = ActionGroup::ALL;
        mi->m_name = "All";
        m_group_store->append(mi);
    }
    for (const auto &[group, name] : action_group_catalog) {
        auto mi = GroupItem::create();
        mi->m_group = group;
        mi->m_name = name;
        m_group_store->append(mi);
    }

    auto ustring_expression = Gtk::ClosureExpression<Glib::ustring>::create(
            [](const Glib::RefPtr<Glib::ObjectBase> &item) -> Glib::ustring {
                const auto col = std::dynamic_pointer_cast<ActionItem>(item);
                return col ? col->m_name : "";
            });
    auto name_sorter = Gtk::StringSorter::create(ustring_expression);
    m_filter = Filter::create();


    auto sort_model = Gtk::SortListModel::create(m_store, name_sorter);

    auto filter_model = Gtk::FilterListModel::create(sort_model, m_filter);


    m_selection = Gtk::SingleSelection::create(filter_model);
    m_group_selection = Gtk::SingleSelection::create(m_group_store);
    m_group_selection->signal_selection_changed().connect([this](guint, guint) {
        auto sel = m_group_selection->get_selected_item();
        auto tr = std::dynamic_pointer_cast<GroupItem>(sel);
        if (!tr)
            return;

        m_filter->set_group(tr->m_group);
    });
    m_view->set_model(m_selection);
    m_group_view->set_model(m_group_selection);


    {
        auto factory = Gtk::SignalListItemFactory::create();
        factory->signal_setup().connect([&](const Glib::RefPtr<Gtk::ListItem> &li) {
            auto label = Gtk::make_managed<Gtk::Label>();
            label->set_ellipsize(Pango::EllipsizeMode::END);

            label->set_xalign(0);
            li->set_child(*label);
        });
        factory->signal_bind().connect([&](const Glib::RefPtr<Gtk::ListItem> &li) {
            auto col = std::dynamic_pointer_cast<ActionItem>(li->get_item());
            if (!col)
                return;
            auto label = dynamic_cast<Gtk::Label *>(li->get_child());
            if (!label)
                return;
            label->set_text(col->m_name);
        });
        auto col = Gtk::ColumnViewColumn::create("Action", factory);
        col->set_expand(true);
        col->set_sorter(name_sorter);
        m_view->append_column(col);
    }
    {
        auto factory = Gtk::SignalListItemFactory::create();
        factory->signal_setup().connect([&](const Glib::RefPtr<Gtk::ListItem> &li) {
            auto label = Gtk::make_managed<Gtk::Label>();
            label->set_ellipsize(Pango::EllipsizeMode::END);
            label->set_xalign(0);
            li->set_child(*label);

            auto text_expr =
                    Gtk::PropertyExpression<Glib::RefPtr<Glib::ObjectBase>>::create(Gtk::ListItem::get_type(), "item");
            auto keys_expr = Gtk::PropertyExpression<Glib::ustring>::create(ActionItem ::get_type(), text_expr, "keys");
            keys_expr->bind(label->property_label(), li);
        });

        auto col = Gtk::ColumnViewColumn::create("Keys", factory);
        m_view->append_column(col);
    }

    {
        auto factory = Gtk::SignalListItemFactory::create();
        factory->signal_setup().connect([&](const Glib::RefPtr<Gtk::ListItem> &li) {
            auto label = Gtk::make_managed<Gtk::Label>();
            label->set_xalign(0);
            li->set_child(*label);
        });
        factory->signal_bind().connect([&](const Glib::RefPtr<Gtk::ListItem> &li) {
            auto col = std::dynamic_pointer_cast<GroupItem>(li->get_item());
            if (!col)
                return;
            auto label = dynamic_cast<Gtk::Label *>(li->get_child());
            if (!label)
                return;
            label->set_text(col->m_name);
        });
        auto col = Gtk::ColumnViewColumn::create("Group", factory);
        col->set_expand(true);
        m_group_view->append_column(col);
    }

    search_entry->signal_activate().connect(sigc::mem_fun(*this, &ToolPopover::emit_tool_activated));
    m_view->signal_activate().connect([this](auto index) { emit_tool_activated(); });
    m_view->set_hexpand(true);


    m_sc = Gtk::make_managed<Gtk::ScrolledWindow>();
    m_sc->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    m_sc->set_min_content_height(210);
    m_sc->set_hexpand(true);
    m_sc->set_has_frame(true);
    m_sc->set_child(*m_view);


    auto box2 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);


    auto sc2 = Gtk::manage(new Gtk::ScrolledWindow());
    sc2->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    sc2->set_child(*m_group_view);
    sc2->set_has_frame(true);
    sc2->set_margin_end(5);

    m_revealer = Gtk::manage(new Gtk::Revealer);
    m_revealer->set_child(*sc2);
    m_revealer->set_transition_type(Gtk::RevealerTransitionType::SLIDE_RIGHT);
    m_revealer->set_reveal_child(true);

    box2->append(*m_revealer);

    box2->append(*m_sc);
    box->append(*box2);

    box->set_margin_start(5);
    box->set_margin_end(5);
    box->set_margin_top(5);

    set_child(*box);
}

void ToolPopover::emit_tool_activated()
{
    auto col = std::dynamic_pointer_cast<ActionItem>(m_selection->get_selected_item());
    if (col) {
        popdown();
        m_signal_action_activated.emit(col->m_id);
    }
}

void ToolPopover::set_can_begin(const std::map<ActionToolID, bool> &can_begin)
{
    for (size_t i = 0; i < m_store->get_n_items(); i++) {
        auto it = m_store->get_item(i);
        if (can_begin.contains(it->m_id))
            it->m_can_begin = can_begin.at(it->m_id);
        else
            it->m_can_begin = true;
    }
}

void ToolPopover::set_key_sequences(ActionToolID action_id, const std::vector<KeySequence> &seqs)
{
    for (size_t i = 0; i < m_store->get_n_items(); i++) {
        auto it = m_store->get_item(i);
        if (it->m_id == action_id)
            it->m_keys = key_sequences_to_string(seqs);
    }
}

void ToolPopover::on_show()
{
    Gtk::Popover::on_show();
    search_entry->grab_focus();
    m_filter->refilter();
}
} // namespace dune3d

#include "select_groups_dialog.hpp"
#include "document/document.hpp"
#include "document/group/group.hpp"
#include <iostream>

namespace dune3d {

SelectGroupsDialog *SelectGroupsDialog::create(const Document &doc, const UUID &current_group,
                                               const std::vector<UUID> &sources)
{
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/dune3d/dune3d/widgets/select_groups_dialog.ui");
    auto w = Gtk::Builder::get_widget_derived<SelectGroupsDialog>(x, "window", doc, current_group, sources);
    w->reference();
    return w;
}

template <typename T> void get_widget(T *&widget, const Glib::RefPtr<Gtk::Builder> &x, const char *name)
{
    widget = x->get_widget<T>(name);
}

#define GET_WIDGET(n)                                                                                                  \
    do {                                                                                                               \
        get_widget(m_##n, x, #n);                                                                                      \
    } while (0)

namespace {
class SelectGroupItem : public Glib::Object {
public:
    static Glib::RefPtr<SelectGroupItem> create()
    {
        return Glib::make_refptr_for_instance<SelectGroupItem>(new SelectGroupItem);
    }

    UUID m_uuid;
    Glib::Property<Glib::ustring> m_name;

private:
    SelectGroupItem() : Glib::ObjectBase("SelectGroupItem"), m_name(*this, "name")
    {
    }
};
} // namespace

class SelectGroupsDialog::Filter : public Gtk::Filter {
public:
    static auto create(const std::vector<UUID> &source_groups)
    {
        return std::shared_ptr<Filter>(new Filter(source_groups));
    }


    void refilter()
    {
        changed();
    }

    bool match_vfunc(const Glib::RefPtr<Glib::ObjectBase> &item) override
    {
        const auto it = std::dynamic_pointer_cast<SelectGroupItem>(item);
        if (!it)
            return false;

        const auto in_source_group = std::ranges::find(m_source_groups, it->m_uuid) != m_source_groups.end();

        return !in_source_group;
    }

private:
    Filter(const std::vector<UUID> &source_groups) : m_source_groups(source_groups)
    {
    }

    const std::vector<UUID> &m_source_groups;
};

SelectGroupsDialog::SelectGroupsDialog(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                       const Document &doc, const UUID &current_group_uu,
                                       const std::vector<UUID> &sources)
    : Gtk::Window(cobject), m_doc(doc), m_source_groups(sources)
{
    set_modal(true);

    GET_WIDGET(ok_button);
    GET_WIDGET(cancel_button);
    GET_WIDGET(available_groups_view);
    GET_WIDGET(source_groups_view);
    GET_WIDGET(include_button);
    GET_WIDGET(exclude_button);
    GET_WIDGET(move_up_button);
    GET_WIDGET(move_down_button);

    m_include_button->signal_clicked().connect(sigc::mem_fun(*this, &SelectGroupsDialog::on_include));
    m_exclude_button->signal_clicked().connect(sigc::mem_fun(*this, &SelectGroupsDialog::on_exclude));
    m_move_up_button->signal_clicked().connect(sigc::mem_fun(*this, &SelectGroupsDialog::on_move_up));
    m_move_down_button->signal_clicked().connect(sigc::mem_fun(*this, &SelectGroupsDialog::on_move_down));

    {
        auto sg = Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::HORIZONTAL);
        sg->add_widget(*m_cancel_button);
        sg->add_widget(*m_ok_button);
    }

    m_cancel_button->signal_clicked().connect([this] { close(); });
    m_ok_button->signal_clicked().connect([this] {
        m_signal_changed.emit();
        close();
    });

    auto groups_sorted = m_doc.get_groups_sorted();

    const auto &current_group = m_doc.get_group(current_group_uu);
    auto new_store = Gio::ListStore<SelectGroupItem>::create();
    for (auto group : groups_sorted) {
        if (group->get_index() > current_group.get_index())
            break;
        if (!group->m_active_wrkpl)
            continue;
        auto gi = SelectGroupItem::create();
        gi->m_name = group->m_name;
        gi->m_uuid = group->m_uuid;
        new_store->append(gi);
    }

    m_source_selection_model = Gtk::SingleSelection::create();
    m_available_selection_model = Gtk::SingleSelection::create();

    m_filter = Filter::create(m_source_groups);
    auto filter_model = Gtk::FilterListModel::create(new_store, m_filter);

    m_available_selection_model->set_model(filter_model);


    auto factory =
            gtk_builder_list_item_factory_new_from_resource(NULL, "/org/dune3d/dune3d/widgets/select_group_item.ui");
    m_available_groups_view->set_factory(Glib::wrap(factory));
    m_available_groups_view->set_model(m_available_selection_model);

    m_source_groups_view->set_factory(Glib::wrap(factory));
    m_source_groups_view->set_model(m_source_selection_model);

    update();
    m_source_selection_model->signal_selection_changed().connect([this](guint, guint) { update_sensitivity(); });
    m_available_selection_model->signal_selection_changed().connect([this](guint, guint) { update_sensitivity(); });
}

void SelectGroupsDialog::update()
{
    auto new_store = Gio::ListStore<SelectGroupItem>::create();
    for (const auto &uu : m_source_groups) {
        auto gi = SelectGroupItem::create();
        const auto &group = m_doc.get_group(uu);
        gi->m_name = group.m_name;
        gi->m_uuid = group.m_uuid;
        new_store->append(gi);
    }
    m_source_selection_model->set_model(new_store);
    m_ok_button->set_sensitive(m_source_groups.size() >= 2);

    m_filter->refilter();

    update_sensitivity();
}

void SelectGroupsDialog::update_sensitivity()
{
    m_include_button->set_sensitive(m_available_selection_model->get_selected() != GTK_INVALID_LIST_POSITION);

    auto sel = m_source_selection_model->get_selected_item();
    auto it = std::dynamic_pointer_cast<SelectGroupItem>(sel);
    if (it) {
        m_exclude_button->set_sensitive(true);
        auto sel_pos = m_source_selection_model->get_selected();
        m_move_up_button->set_sensitive(sel_pos != 0);
        m_move_down_button->set_sensitive(sel_pos != m_source_selection_model->get_n_items() - 1);
    }
    else {
        m_exclude_button->set_sensitive(false);
        m_move_down_button->set_sensitive(false);
        m_move_up_button->set_sensitive(false);
    }
}

void SelectGroupsDialog::on_include()
{
    auto sel = m_available_selection_model->get_selected_item();
    auto it = std::dynamic_pointer_cast<SelectGroupItem>(sel);
    if (!it)
        return;

    m_source_groups.push_back(it->m_uuid);
    update();
}

void SelectGroupsDialog::on_exclude()
{
    auto sel = m_source_selection_model->get_selected_item();
    auto it = std::dynamic_pointer_cast<SelectGroupItem>(sel);
    if (!it)
        return;

    auto p = std::ranges::find(m_source_groups, it->m_uuid);
    if (p == m_source_groups.end())
        return;

    m_source_groups.erase(p);
    update();
}

void SelectGroupsDialog::on_move_up()
{
    auto sel = m_source_selection_model->get_selected_item();
    const auto sel_pos = m_source_selection_model->get_selected();
    auto it = std::dynamic_pointer_cast<SelectGroupItem>(sel);
    if (!it)
        return;

    auto p = std::ranges::find(m_source_groups, it->m_uuid);
    if (p == m_source_groups.end())
        return;
    if (p == m_source_groups.begin())
        return;

    std::swap(*p, *std::prev(p));
    update();
    m_source_selection_model->set_selected(sel_pos - 1);
}

void SelectGroupsDialog::on_move_down()
{
    auto sel = m_source_selection_model->get_selected_item();
    const auto sel_pos = m_source_selection_model->get_selected();
    auto it = std::dynamic_pointer_cast<SelectGroupItem>(sel);
    if (!it)
        return;

    auto p = std::ranges::find(m_source_groups, it->m_uuid);
    if (p == m_source_groups.end())
        return;
    if (p == std::prev(m_source_groups.end()))
        return;

    std::swap(*p, *std::next(p));
    update();
    m_source_selection_model->set_selected(sel_pos + 1);
}

std::vector<UUID> SelectGroupsDialog::get_selected_groups() const
{
    return m_source_groups;
}


} // namespace dune3d

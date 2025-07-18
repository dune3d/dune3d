#include "select_group_dialog.hpp"
#include "document/document.hpp"
#include "document/group/group.hpp"
#include "document/group/igroup_solid_model.hpp"
#include "util/gtk_util.hpp"

namespace dune3d {

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
SelectGroupDialog::SelectGroupDialog(const Document &doc, const UUID &current_group_uu, const UUID &source,
                                     GroupsFilter groups_filter)
    : Gtk::Window(), m_doc(doc)
{
    set_modal(true);
    set_title("Select Group");
    set_default_size(-1, 300);

    auto headerbar = Gtk::make_managed<Gtk::HeaderBar>();
    headerbar->set_show_title_buttons(false);
    set_titlebar(*headerbar);
    install_esc_to_close(*this);

    auto sg = Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::HORIZONTAL);

    m_cancel_button = Gtk::make_managed<Gtk::Button>("Cancel");
    headerbar->pack_start(*m_cancel_button);
    m_cancel_button->signal_clicked().connect([this] { close(); });
    sg->add_widget(*m_cancel_button);


    m_ok_button = Gtk::make_managed<Gtk::Button>("OK");
    m_ok_button->add_css_class("suggested-action");
    headerbar->pack_end(*m_ok_button);
    sg->add_widget(*m_ok_button);

    m_cancel_button->signal_clicked().connect([this] { close(); });
    m_ok_button->signal_clicked().connect([this] {
        m_signal_changed.emit();
        close();
    });

    auto groups_sorted = m_doc.get_groups_sorted();

    const auto &current_group = m_doc.get_group(current_group_uu);
    auto new_store = Gio::ListStore<SelectGroupItem>::create();
    unsigned int index = 0;
    unsigned int selected_index = 0;
    for (auto group : groups_sorted) {
        if (group->get_index() >= current_group.get_index())
            break;
        if (groups_filter == GroupsFilter::SOLID_MODEL)
            if (!IGroupSolidModel::try_get_solid_model(*group))
                continue;
        auto gi = SelectGroupItem::create();
        gi->m_name = group->m_name;
        gi->m_uuid = group->m_uuid;
        new_store->append(gi);
        if (group->m_uuid == source)
            selected_index = index;
        index++;
    }

    m_selection_model = Gtk::SingleSelection::create();
    m_selection_model->set_can_unselect(false);

    m_selection_model->set_model(new_store);

    auto sc = Gtk::make_managed<Gtk::ScrolledWindow>();
    sc->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);

    m_groups_view = Gtk::make_managed<Gtk::ListView>();
    sc->set_child(*m_groups_view);
    set_child(*sc);

    auto factory =
            gtk_builder_list_item_factory_new_from_resource(NULL, "/org/dune3d/dune3d/widgets/select_group_item.ui");
    m_groups_view->set_factory(Glib::wrap(factory));
    m_groups_view->set_model(m_selection_model);

    m_selection_model->set_selected(selected_index);
    m_groups_view->signal_activate().connect([this](int) {
        m_signal_changed.emit();
        close();
    });
}

UUID SelectGroupDialog::get_selected_group() const
{
    auto sel = m_selection_model->get_selected_item();
    auto it = std::dynamic_pointer_cast<SelectGroupItem>(sel);
    if (it)
        return it->m_uuid;

    return {};
}


} // namespace dune3d

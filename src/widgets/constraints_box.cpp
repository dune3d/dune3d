#include "constraints_box.hpp"
#include "core/core.hpp"
#include "document/document.hpp"
#include "document/constraint/constraint.hpp"
#include "document/group/group.hpp"
#include "util/template_util.hpp"

namespace dune3d {

class ConstraintItem : public Glib::Object {
public:
    static Glib::RefPtr<ConstraintItem> create()
    {
        return Glib::make_refptr_for_instance<ConstraintItem>(new ConstraintItem);
    }

    UUID m_uuid;
    Glib::Property<Glib::ustring> m_name;

private:
    ConstraintItem() : Glib::ObjectBase("ConstraintItem"), m_name(*this, "name")
    {
    }
};


ConstraintsBox::ConstraintsBox(Core &core) : Gtk::Box(Gtk::Orientation::VERTICAL), m_core(core)
{
    m_selection_model = Gtk::SingleSelection::create();

    auto factory = gtk_builder_list_item_factory_new_from_resource(
            NULL, "/org/dune3d/dune3d/widgets/constraints_box_list_item.ui");

    m_view = Gtk::make_managed<Gtk::ListView>(m_selection_model, Glib::wrap(GTK_LIST_ITEM_FACTORY(factory)));
    {
        auto controller = Gtk::EventControllerKey::create();
        controller->signal_key_pressed().connect(
                [this](guint keyval, guint keycode, Gdk::ModifierType state) {
                    if (keyval == GDK_KEY_Delete || keyval == GDK_KEY_KP_Delete) {
                        auto sel = m_selection_model->get_selected_item();
                        auto it = std::dynamic_pointer_cast<ConstraintItem>(sel);
                        if (it) {
                            m_core.get_current_document().m_constraints.erase(it->m_uuid);
                            m_core.get_current_document().set_group_solve_pending(m_core.get_current_group());
                            m_core.set_needs_save();
                            m_core.rebuild("delete constraint");
                            m_signal_changed.emit();
                        }
                        return true;
                    }
                    return false;
                },
                true);

        m_view->add_controller(controller);
    }
    m_selection_model->signal_selection_changed().connect([this](guint, guint) {
        auto sel = std::dynamic_pointer_cast<ConstraintItem>(m_selection_model->get_selected_item());
        m_signal_constraint_selected.emit(sel->m_uuid);
    });
    m_view->add_css_class("navigation-sidebar");
    m_redundant_only_cb = Gtk::make_managed<Gtk::CheckButton>("Show only redundant constraints");
    m_redundant_only_cb->set_sensitive(false);
    auto sc = Gtk::make_managed<Gtk::ScrolledWindow>();
    sc->set_vexpand(true);
    append(*sc);
    {
        auto sep = Gtk::make_managed<Gtk::Separator>();
        append(*sep);
    }
    append(*m_redundant_only_cb);
    sc->set_child(*m_view);
    sc->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);

    m_redundant_only_cb->signal_toggled().connect([this] { update_internal(); });
}

void ConstraintsBox::update()
{
    auto new_store = Gio::ListStore<ConstraintItem>::create();
    if (!m_core.has_documents()) {
        m_updating = true;
        m_selection_model->set_model(new_store);
        m_redundant_only_cb->set_sensitive(false);
        m_redundant_only_cb->set_active(false);
        m_updating = false;
        return;
    }
    auto &doc = m_core.get_current_document();
    auto &group = doc.get_group(m_core.get_current_group());

    const auto has_redundant =
            any_of(group.m_solve_result, SolveResult::REDUNDANT_OKAY, SolveResult::REDUNDANT_DIDNT_CONVERGE);
    m_redundant_only_cb->set_sensitive(has_redundant);
    m_updating = true;
    m_redundant_only_cb->set_active(false);
    m_updating = false;

    update_internal();
}

void ConstraintsBox::update_internal()
{
    if (m_updating)
        return;
    auto &doc = m_core.get_current_document();
    auto &group = doc.get_group(m_core.get_current_group());
    std::vector<Constraint *> constraints;

    if (m_redundant_only_cb->get_active()) {
        auto bad = group.find_redundant_constraints(doc);
        for (const auto &uu : bad) {
            constraints.push_back(&doc.get_constraint(uu));
        }
    }
    else {
        for (const auto &[uu, it] : m_core.get_current_document().m_constraints) {
            if (it->m_group != group.m_uuid)
                continue;
            constraints.push_back(it.get());
        }
    }
    std::ranges::sort(constraints, [](auto a, auto b) { return a->get_type() < b->get_type(); });
    auto new_store = Gio::ListStore<ConstraintItem>::create();

    for (const auto it : constraints) {
        auto ci = ConstraintItem::create();
        ci->m_name = it->get_type_name();
        ci->m_uuid = it->m_uuid;
        new_store->append(ci);
    }
    m_selection_model->set_model(new_store);
}

} // namespace dune3d

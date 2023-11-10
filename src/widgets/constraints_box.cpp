#include "constraints_box.hpp"
#include "core/core.hpp"
#include "document/document.hpp"
#include "document/constraint/constraint.hpp"

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


ConstraintsBox::ConstraintsBox(Core &core) : m_core(core)
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
    set_child(*m_view);
    set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
}

void ConstraintsBox::update()
{
    auto new_store = Gio::ListStore<ConstraintItem>::create();
    if (!m_core.has_documents()) {
        m_selection_model->set_model(new_store);
        return;
    }
    auto group_uu = m_core.get_current_group();

    std::vector<Constraint *> constraints;

    for (const auto &[uu, it] : m_core.get_current_document().m_constraints) {
        if (it->m_group != group_uu)
            continue;
        constraints.push_back(it.get());
    }
    std::ranges::sort(constraints, [](auto a, auto b) { return a->get_type() < b->get_type(); });
    for (const auto it : constraints) {
        auto ci = ConstraintItem::create();
        ci->m_name = it->get_type_name();
        ci->m_uuid = it->m_uuid;
        new_store->append(ci);
    }
    m_selection_model->set_model(new_store);
}

} // namespace dune3d

#include "constraints_box.hpp"
#include "core/core.hpp"
#include "document/document.hpp"
#include "document/constraint.hpp"

namespace dune3d {

class ConstraintsBox::ConstraintItem : public Glib::Object {
public:
    static Glib::RefPtr<ConstraintItem> create()
    {
        return Glib::make_refptr_for_instance<ConstraintItem>(new ConstraintItem);
    }

    UUID m_uuid;
    std::string m_name;

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
    ConstraintItem() : Glib::ObjectBase("ConstraintItem")
    {
    }

    static GType gtype;
};

GType ConstraintsBox::ConstraintItem::gtype;


ConstraintsBox::ConstraintsBox(Core &core) : m_core(core)
{

    m_store = Gio::ListStore<ConstraintItem>::create();


    auto selection_model = Gtk::SingleSelection::create(m_store);


    auto factory = Gtk::SignalListItemFactory::create();
    factory->signal_setup().connect([&](const Glib::RefPtr<Gtk::ListItem> &li) {
        auto label = Gtk::make_managed<Gtk::Label>();
        label->set_xalign(0);
        li->set_child(*label);
    });
    factory->signal_bind().connect([&](const Glib::RefPtr<Gtk::ListItem> &li) {
        auto col = std::dynamic_pointer_cast<ConstraintItem>(li->get_item());
        if (!col)
            return;
        auto label = dynamic_cast<Gtk::Label *>(li->get_child());
        if (!label)
            return;
        label->set_text(col->m_name);
    });


    m_view = Gtk::make_managed<Gtk::ListView>(selection_model, factory);
    {
        auto controller = Gtk::EventControllerKey::create();
        controller->signal_key_pressed().connect(
                [this, selection_model](guint keyval, guint keycode, Gdk::ModifierType state) {
                    if (keyval == GDK_KEY_Delete || keyval == GDK_KEY_KP_Delete) {
                        auto sel = selection_model->get_selected_item();
                        auto it = std::dynamic_pointer_cast<ConstraintItem>(sel);
                        if (it) {
                            m_core.get_current_document().m_constraints.erase(it->m_uuid);
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
    selection_model->signal_selection_changed().connect([this, selection_model](guint, guint) {
        auto sel = selection_model->get_selected_item();
        auto tr = std::dynamic_pointer_cast<Gtk::TreeListRow>(sel);
    });
    m_view->add_css_class("navigation-sidebar");
    set_child(*m_view);
    set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
}

void ConstraintsBox::update()
{
    m_store->remove_all();
    if (!m_core.has_documents())
        return;
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
        m_store->append(ci);
    }
}

} // namespace dune3d

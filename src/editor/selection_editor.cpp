#include "selection_editor.hpp"
#include "core/core.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/constraint/constraint.hpp"
#include "util/selection_util.hpp"
#include "util/gtk_util.hpp"
#include "widgets/spin_button_dim.hpp"
#include <variant>

namespace dune3d {
SelectionEditor::SelectionEditor(Core &core) : Gtk::Box(Gtk::Orientation::VERTICAL), m_core(core)
{
    m_title = Gtk::make_managed<Gtk::Label>();
    m_title->set_margin(10);
    m_title->set_margin_bottom(0);
    m_title->set_xalign(0);
    append(*m_title);
}

class GenericEditor : public Gtk::Grid {
public:
    GenericEditor(Document &doc, const std::set<SelectableRef> &sel)
    {
        set_row_spacing(5);
        set_column_spacing(5);
        using ItemType = std::variant<Entity::Type, Constraint::Type>;
        std::set<std::pair<SelectableRef::Type, UUID>> items;
        for (const auto &sr : sel) {
            items.emplace(sr.type, sr.item);
        }
        std::map<ItemType, unsigned int> item_count;
        for (const auto &[type, uu] : items) {
            if (type == SelectableRef::Type::ENTITY) {
                auto &en = doc.get_entity(uu);
                item_count[en.get_type()]++;
            }
            else if (type == SelectableRef::Type::CONSTRAINT) {
                auto &co = doc.get_constraint(uu);
                item_count[co.get_type()]++;
            }
        }
        int top = 0;
        for (const auto &[type, count] : item_count) {
            std::string type_name;
            if (auto entity_type = std::get_if<Entity::Type>(&type)) {
                type_name = Entity::get_type_name(*entity_type);
            }
            else if (auto constraint_type = std::get_if<Constraint::Type>(&type)) {
                type_name = Constraint::get_type_name(*constraint_type);
            }
            {
                auto la = Gtk::make_managed<Gtk::Label>(type_name);
                la->set_xalign(1);
                attach(*la, 0, top);
            }
            {
                auto la = Gtk::make_managed<Gtk::Label>(std::to_string(count));
                la->set_xalign(0);
                attach(*la, 1, top++);
            }
        }
    }
};

class WorkplaneEditor : public Gtk::Grid, public Changeable {
public:
    WorkplaneEditor(Document &doc, const UUID &wrkpl) : m_doc(doc), m_wrkpl(doc.get_entity<EntityWorkplane>(wrkpl))
    {
        set_row_spacing(5);
        set_column_spacing(5);

        m_width_sp = Gtk::make_managed<SpinButtonDim>();
        m_width_sp->set_hexpand(true);
        m_width_sp->set_range(1, 1000);
        m_width_sp->set_value(m_wrkpl.m_size.x);
        spinbutton_connect_activate_immediate(*m_width_sp, [this] {
            m_wrkpl.m_size.x = m_width_sp->get_value();
            m_signal_changed.emit();
        });
        int top = 0;
        grid_attach_label_and_widget(*this, "Width", *m_width_sp, top);

        m_height_sp = Gtk::make_managed<SpinButtonDim>();
        m_height_sp->set_range(1, 1000);
        m_height_sp->set_value(m_wrkpl.m_size.y);
        spinbutton_connect_activate_immediate(*m_height_sp, [this] {
            m_wrkpl.m_size.y = m_height_sp->get_value();
            m_signal_changed.emit();
        });
        grid_attach_label_and_widget(*this, "Height", *m_height_sp, top);
    }

private:
    Document &m_doc;
    EntityWorkplane &m_wrkpl;

    SpinButtonDim *m_width_sp = nullptr;
    SpinButtonDim *m_height_sp = nullptr;
};

void SelectionEditor::set_selection(const std::set<SelectableRef> &sel)
{
    if (m_editor) {
        remove(*m_editor);
        m_editor = nullptr;
    }
    if (!m_core.has_documents())
        return;

    auto wrkpl = entity_from_selection(m_core.get_current_document(), sel, Entity::Type::WORKPLANE);
    if (wrkpl) {
        m_title->set_label("Workplane");
        m_title->set_tooltip_text((std::string)*wrkpl);
        auto ed = Gtk::make_managed<WorkplaneEditor>(m_core.get_current_document(), *wrkpl);
        m_editor = ed;
        ed->signal_changed().connect([this] { m_signal_changed.emit(); });
    }
    else if (sel.size()) {
        m_title->set_label("");
        m_editor = Gtk::make_managed<GenericEditor>(m_core.get_current_document(), sel);
    }

    if (!m_editor) {
        m_editor = Gtk::make_managed<Gtk::Label>("No selection");
        m_editor->set_vexpand(true);
        m_title->set_label("");
    }

    m_editor->set_margin(10);

    append(*m_editor);
}

} // namespace dune3d

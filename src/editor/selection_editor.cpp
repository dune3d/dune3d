#include "selection_editor.hpp"
#include "core/core.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/entity/entity_step.hpp"
#include "document/entity/entity_document.hpp"
#include "document/constraint/constraint.hpp"
#include "util/selection_util.hpp"
#include "util/gtk_util.hpp"
#include "util/fs_util.hpp"
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
                type_name = Entity::get_type_name_for_n(*entity_type, count);
            }
            else if (auto constraint_type = std::get_if<Constraint::Type>(&type)) {
                type_name = Constraint::get_type_name(*constraint_type);
            }
            type_name += ":";
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
    WorkplaneEditor(Document &doc, const UUID &wrkpl) : m_wrkpl(doc.get_entity<EntityWorkplane>(wrkpl))
    {
        set_row_spacing(5);
        set_column_spacing(5);
        int top = 0;


        m_name_entry = Gtk::make_managed<Gtk::Entry>();
        m_name_entry->set_text(m_wrkpl.m_name);
        if (m_wrkpl.m_kind != ItemKind::USER) {
            m_name_entry->set_sensitive(false);
        }
        else {
            m_name_entry->signal_activate().connect([this] {
                m_wrkpl.m_name = m_name_entry->get_text();
                m_signal_changed.emit();
            });
        }
        grid_attach_label_and_widget(*this, "Name", *m_name_entry, top);


        m_width_sp = Gtk::make_managed<SpinButtonDim>();
        m_width_sp->set_hexpand(true);
        m_width_sp->set_range(1, 1000);
        m_width_sp->set_value(m_wrkpl.m_size.x);
        spinbutton_connect_activate_immediate(*m_width_sp, [this] {
            m_wrkpl.m_size.x = m_width_sp->get_value();
            m_signal_changed.emit();
        });
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
    EntityWorkplane &m_wrkpl;

    Gtk::Entry *m_name_entry = nullptr;
    SpinButtonDim *m_width_sp = nullptr;
    SpinButtonDim *m_height_sp = nullptr;
};

class STEPEditor : public Gtk::Grid, public Changeable {
public:
    STEPEditor(const std::filesystem::path &document_dir, EntitySTEP &step) : m_doc_dir(document_dir), m_step(step)
    {
        set_row_spacing(5);
        set_column_spacing(5);

        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
        m_path_entry = Gtk::make_managed<Gtk::Entry>();
        m_path_entry->set_hexpand(true);
        m_path_button = Gtk::make_managed<Gtk::Button>();
        m_path_button->set_image_from_icon_name("document-open-symbolic");
        box->append(*m_path_entry);
        box->append(*m_path_button);
        box->add_css_class("linked");
        m_path_entry->set_text(path_to_string(step.m_path));
        m_path_entry->signal_activate().connect([this] { update_step(); });
        m_path_button->signal_clicked().connect([this] {
            auto dialog = Gtk::FileDialog::create();

            // Add filters, so that only certain file types can be selected:
            auto filters = Gio::ListStore<Gtk::FileFilter>::create();

            auto filter_any = Gtk::FileFilter::create();
            filter_any->set_name("STEP");
            filter_any->add_pattern("*.step");
            filter_any->add_pattern("*.STEP");
            filter_any->add_pattern("*.stp");
            filter_any->add_pattern("*.STP");
            filters->append(filter_any);

            dialog->set_filters(filters);

            // Show the dialog and wait for a user response:
            dialog->open(*dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW)),
                         [this, dialog](const Glib::RefPtr<Gio::AsyncResult> &result) {
                             try {
                                 auto file = dialog->open_finish(result);
                                 // Notice that this is a std::string, not a Glib::ustring.
                                 auto path = path_from_string(file->get_path());
                                 if (auto rel = get_relative_filename(path, m_doc_dir)) {
                                     m_path_entry->set_text(path_to_string(*rel));
                                 }
                                 else {
                                     m_path_entry->set_text(path_to_string(path));
                                 }
                                 update_step();
                             }
                             catch (const Gtk::DialogError &err) {
                             }
                             catch (const Glib::Error &err) {
                             }
                         });
        });

        int top = 0;
        grid_attach_label_and_widget(*this, "Path", *box, top);
    }

private:
    const std::filesystem::path m_doc_dir;
    EntitySTEP &m_step;
    void update_step()
    {
        m_step.m_path = path_from_string(m_path_entry->get_text());
        m_step.update_imported(m_doc_dir);
        m_signal_changed.emit();
    }


    Gtk::Entry *m_path_entry = nullptr;
    Gtk::Button *m_path_button = nullptr;
};

class DocumentEditor : public Gtk::Grid, public Changeable {
public:
    DocumentEditor(const std::filesystem::path &document_dir, EntityDocument &doc) : m_doc_dir(document_dir), m_doc(doc)
    {
        set_row_spacing(5);
        set_column_spacing(5);

        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
        m_path_entry = Gtk::make_managed<Gtk::Entry>();
        m_path_entry->set_hexpand(true);
        m_path_button = Gtk::make_managed<Gtk::Button>();
        m_path_button->set_image_from_icon_name("document-open-symbolic");
        box->append(*m_path_entry);
        box->append(*m_path_button);
        box->add_css_class("linked");
        m_path_entry->set_text(path_to_string(doc.m_path));
        m_path_entry->signal_activate().connect([this] { update_doc(); });
        m_path_button->signal_clicked().connect([this] {
            auto dialog = Gtk::FileDialog::create();

            // Add filters, so that only certain file types can be selected:
            auto filters = Gio::ListStore<Gtk::FileFilter>::create();

            auto filter_any = Gtk::FileFilter::create();
            filter_any->set_name("Dune 3D document");
            filter_any->add_pattern("*.d3ddoc");
            filters->append(filter_any);

            dialog->set_filters(filters);

            // Show the dialog and wait for a user response:
            dialog->open(*dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW)),
                         [this, dialog](const Glib::RefPtr<Gio::AsyncResult> &result) {
                             try {
                                 auto file = dialog->open_finish(result);
                                 // Notice that this is a std::string, not a Glib::ustring.
                                 auto path = path_from_string(file->get_path());
                                 if (auto rel = get_relative_filename(path, m_doc_dir)) {
                                     m_path_entry->set_text(path_to_string(*rel));
                                 }
                                 else {
                                     m_path_entry->set_text(path_to_string(path));
                                 }
                                 update_doc();
                             }
                             catch (const Gtk::DialogError &err) {
                             }
                             catch (const Glib::Error &err) {
                             }
                         });
        });

        int top = 0;
        grid_attach_label_and_widget(*this, "Path", *box, top);
    }

private:
    const std::filesystem::path m_doc_dir;
    EntityDocument &m_doc;
    void update_doc()
    {
        m_doc.m_path = path_from_string(m_path_entry->get_text());
        m_signal_changed.emit();
    }


    Gtk::Entry *m_path_entry = nullptr;
    Gtk::Button *m_path_button = nullptr;
};

void SelectionEditor::set_selection(const std::set<SelectableRef> &sel)
{
    if (m_editor) {
        remove(*m_editor);
        m_editor = nullptr;
    }
    if (m_core.has_documents()) {
        if (auto wrkpl = entity_and_point_from_selection(m_core.get_current_document(), sel, Entity::Type::WORKPLANE)) {
            m_title->set_label("Workplane");
            m_title->set_tooltip_text((std::string)wrkpl->entity);
            auto ed = Gtk::make_managed<WorkplaneEditor>(m_core.get_current_document(), wrkpl->entity);
            m_editor = ed;
            ed->signal_changed().connect([this] { m_signal_changed.emit(); });
        }
        else if (auto step = entity_and_point_from_selection(m_core.get_current_document(), sel, Entity::Type::STEP)) {
            m_title->set_label("STEP");
            m_title->set_tooltip_text((std::string)step->entity);
            auto ed = Gtk::make_managed<STEPEditor>(m_core.get_current_document_directory(),
                                                    m_core.get_current_document().get_entity<EntitySTEP>(step->entity));
            m_editor = ed;
            ed->signal_changed().connect([this] { m_signal_changed.emit(); });
        }
        else if (auto doc =
                         entity_and_point_from_selection(m_core.get_current_document(), sel, Entity::Type::DOCUMENT)) {
            m_title->set_label("Document");
            m_title->set_tooltip_text((std::string)doc->entity);
            auto ed = Gtk::make_managed<DocumentEditor>(
                    m_core.get_current_document_directory(),
                    m_core.get_current_document().get_entity<EntityDocument>(doc->entity));
            m_editor = ed;
            ed->signal_changed().connect([this] { m_signal_changed.emit(); });
        }
        else if (sel.size()) {
            m_title->set_label("");
            m_editor = Gtk::make_managed<GenericEditor>(m_core.get_current_document(), sel);
        }
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

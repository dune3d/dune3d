#include "selection_editor.hpp"
#include "core/core.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/entity/entity_step.hpp"
#include "document/entity/entity_document.hpp"
#include "document/entity/entity_cluster.hpp"
#include "document/entity/entity_text.hpp"
#include "document/entity/entity_picture.hpp"
#include "document/constraint/constraint.hpp"
#include "util/selection_util.hpp"
#include "util/gtk_util.hpp"
#include "util/fs_util.hpp"
#include "util/text_render.hpp"
#include "widgets/spin_button_dim.hpp"
#include "widgets/spin_button_angle.hpp"
#include "workspace/document_view.hpp"
#include "workspace/entity_view.hpp"
#include "idocument_view_provider.hpp"
#include <variant>

namespace dune3d {

static bool update_if_changed(double &target, double src)
{
    if (target == src)
        return false;
    target = src;
    return true;
}

SelectionEditor::SelectionEditor(Core &core, IDocumentViewProvider &prv)
    : Gtk::Box(Gtk::Orientation::VERTICAL), m_core(core), m_doc_view_prv(prv)
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

class WorkplaneEditor : public Gtk::Grid, public ChangeableCommitMode {
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
            connect_entry(*m_name_entry, [this] {
                m_wrkpl.m_name = m_name_entry->get_text();
                m_signal_changed.emit(CommitMode::IMMEDIATE);
            });
        }
        grid_attach_label_and_widget(*this, "Name", *m_name_entry, top);


        m_width_sp = Gtk::make_managed<SpinButtonDim>();
        m_width_sp->set_hexpand(true);
        m_width_sp->set_range(1, 1000);
        m_width_sp->set_value(m_wrkpl.m_size.x);
        connect_spinbutton(*m_width_sp,
                           [this] { return update_if_changed(m_wrkpl.m_size.x, m_width_sp->get_value()); });
        grid_attach_label_and_widget(*this, "Width", *m_width_sp, top);

        m_height_sp = Gtk::make_managed<SpinButtonDim>();
        m_height_sp->set_range(1, 1000);
        m_height_sp->set_value(m_wrkpl.m_size.y);
        connect_spinbutton(*m_height_sp,
                           [this] { return update_if_changed(m_wrkpl.m_size.y, m_height_sp->get_value()); });
        grid_attach_label_and_widget(*this, "Height", *m_height_sp, top);
    }

private:
    EntityWorkplane &m_wrkpl;

    Gtk::Entry *m_name_entry = nullptr;
    SpinButtonDim *m_width_sp = nullptr;
    SpinButtonDim *m_height_sp = nullptr;
};

class STEPEditor : public Gtk::Grid, public ChangeableCommitMode {
public:
    STEPEditor(const std::filesystem::path &document_dir, EntitySTEP &step) : m_doc_dir(document_dir), m_step(step)
    {
        set_row_spacing(5);
        set_column_spacing(5);

        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
        m_path_entry = Gtk::make_managed<Gtk::Entry>();
        m_path_entry->set_hexpand(true);
        auto path_button = Gtk::make_managed<Gtk::Button>();
        path_button->set_image_from_icon_name("document-open-symbolic");
        box->append(*m_path_entry);
        box->append(*path_button);
        box->add_css_class("linked");
        m_path_entry->set_text(path_to_string(step.m_path));
        connect_entry(*m_path_entry, sigc::mem_fun(*this, &STEPEditor::update_step));
        path_button->signal_clicked().connect([this] {
            auto dialog = Gtk::FileDialog::create();
            dialog->set_initial_file(Gio::File::create_for_path(path_to_string(m_step.get_path(m_doc_dir))));

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

        auto reload_button = Gtk::make_managed<Gtk::Button>();
        reload_button->set_image_from_icon_name("view-refresh-symbolic");
        box->append(*reload_button);
        reload_button->signal_clicked().connect([this] {
            m_step.update_imported(m_doc_dir);
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });

        int top = 0;
        grid_attach_label_and_widget(*this, "Path", *box, top);

        auto include_in_solid_model_switch = Gtk::make_managed<Gtk::CheckButton>("In solid model");
        include_in_solid_model_switch->set_halign(Gtk::Align::START);
        include_in_solid_model_switch->set_active(step.m_include_in_solid_model);
        include_in_solid_model_switch->property_active().signal_changed().connect(
                [this, include_in_solid_model_switch] {
                    m_step.m_include_in_solid_model = include_in_solid_model_switch->get_active();
                    m_signal_changed.emit(CommitMode::IMMEDIATE);
                });

        attach(*include_in_solid_model_switch, 1, top++);
    }

private:
    const std::filesystem::path m_doc_dir;
    EntitySTEP &m_step;
    void update_step()
    {
        const auto path = path_from_string(m_path_entry->get_text());
        if (m_step.m_path == path)
            return;
        m_step.m_path = path;
        m_step.update_imported(m_doc_dir);
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    }


    Gtk::Entry *m_path_entry = nullptr;
};

class DocumentEditor : public Gtk::Grid, public ChangeableCommitMode {
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
        connect_entry(*m_path_entry, sigc::mem_fun(*this, &DocumentEditor::update_doc));
        m_path_button->signal_clicked().connect([this] {
            auto dialog = Gtk::FileDialog::create();
            dialog->set_initial_file(Gio::File::create_for_path(path_to_string(m_doc.get_path(m_doc_dir))));

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
        const auto path = path_from_string(m_path_entry->get_text());
        if (m_doc.m_path == path)
            return;
        m_doc.m_path = path;
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    }


    Gtk::Entry *m_path_entry = nullptr;
    Gtk::Button *m_path_button = nullptr;
};

class EntityViewEditorSTEP : public Gtk::Grid, public Changeable {
public:
    EntityViewEditorSTEP(DocumentView &view, const UUID &uu) : m_view(view), m_entity_uuid(uu)
    {
        set_row_spacing(5);
        set_column_spacing(5);

        int top = 0;
        auto items = Gtk::StringList::create();
        items->append("Solid");
        items->append("Hidden");
        items->append("Wireframe");
        items->append("Solid and wireframe");

        m_display_combo = Gtk::make_managed<Gtk::DropDown>(items);
        m_display_combo->set_hexpand(true);
        m_display_combo->set_selected(0);
        if (auto ev = dynamic_cast<const EntityViewSTEP *>(view.get_entity_view(uu))) {
            m_display_combo->set_selected(static_cast<guint>(ev->m_display));
        }
        m_display_combo->property_selected().signal_changed().connect([this] {
            auto ev =
                    dynamic_cast<EntityViewSTEP *>(m_view.get_or_create_entity_view(m_entity_uuid, Entity::Type::STEP));
            if (!ev)
                return;
            ev->m_display = static_cast<EntityViewSTEP::Display>(m_display_combo->get_selected());
            m_signal_changed.emit();
        });
        grid_attach_label_and_widget(*this, "Display", *m_display_combo, top);
    }

private:
    DocumentView &m_view;
    const UUID m_entity_uuid;

    Gtk::DropDown *m_display_combo = nullptr;
};


class ClusterEditor : public Gtk::Grid, public ChangeableCommitMode {
public:
    ClusterEditor(Document &doc, const UUID &cluster) : m_cluster(doc.get_entity<EntityCluster>(cluster))
    {
        set_row_spacing(5);
        set_column_spacing(5);
        int top = 0;


        m_angle_sp = Gtk::make_managed<SpinButtonAngle>();
        m_angle_sp->set_hexpand(true);
        m_angle_sp->set_value(m_cluster.m_angle);
        connect_spinbutton(*m_angle_sp,
                           [this] { return update_if_changed(m_cluster.m_angle, m_angle_sp->get_value()); });
        grid_attach_label_and_widget(*this, "Angle", *m_angle_sp, top);

        m_lock_angle_button = Gtk::make_managed<Gtk::ToggleButton>();
        m_lock_angle_button->set_tooltip_text("Lock angle");
        m_lock_angle_button->set_icon_name("system-lock-screen-symbolic");
        m_lock_angle_button->set_active(m_cluster.m_lock_angle);
        m_lock_angle_button->signal_toggled().connect([this] {
            m_cluster.m_lock_angle = m_lock_angle_button->get_active();
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
        attach(*m_lock_angle_button, 2, top - 1);

        m_scale_x_sp = Gtk::make_managed<Gtk::SpinButton>();
        m_scale_x_sp->set_hexpand(true);
        m_scale_x_sp->set_range(-1e3, 1e3);
        m_scale_x_sp->set_digits(4);
        m_scale_x_sp->set_increments(.1, .1);
        m_scale_x_sp->set_value(m_cluster.m_scale_x);
        connect_spinbutton(*m_scale_x_sp,
                           [this] { return update_if_changed(m_cluster.m_scale_x, m_scale_x_sp->get_value()); });
        grid_attach_label_and_widget(*this, "X scale", *m_scale_x_sp, top);

        m_lock_scale_x_button = Gtk::make_managed<Gtk::ToggleButton>();
        m_lock_scale_x_button->set_tooltip_text("Lock X scale");
        m_lock_scale_x_button->set_icon_name("system-lock-screen-symbolic");
        m_lock_scale_x_button->set_active(m_cluster.m_lock_scale_x);
        m_lock_scale_x_button->signal_toggled().connect([this] {
            m_cluster.m_lock_scale_x = m_lock_scale_x_button->get_active();
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
        attach(*m_lock_scale_x_button, 2, top - 1);

        m_scale_y_sp = Gtk::make_managed<Gtk::SpinButton>();
        m_scale_y_sp->set_hexpand(true);
        m_scale_y_sp->set_range(-1e3, 1e3);
        m_scale_y_sp->set_digits(4);
        m_scale_y_sp->set_increments(.1, .1);
        m_scale_y_sp->set_value(m_cluster.m_scale_y);
        connect_spinbutton(*m_scale_y_sp,
                           [this] { return update_if_changed(m_cluster.m_scale_y, m_scale_y_sp->get_value()); });
        grid_attach_label_and_widget(*this, "Y scale", *m_scale_y_sp, top);

        m_lock_scale_y_button = Gtk::make_managed<Gtk::ToggleButton>();
        m_lock_scale_y_button->set_tooltip_text("Lock Y scale");
        m_lock_scale_y_button->set_icon_name("system-lock-screen-symbolic");
        m_lock_scale_y_button->set_active(m_cluster.m_lock_scale_y);
        m_lock_scale_y_button->signal_toggled().connect([this] {
            m_cluster.m_lock_scale_y = m_lock_scale_y_button->get_active();
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
        attach(*m_lock_scale_y_button, 2, top - 1);

        m_lock_aspect_ratio_button = Gtk::make_managed<Gtk::ToggleButton>();
        m_lock_aspect_ratio_button->set_tooltip_text("Lock aspect ratio");
        m_lock_aspect_ratio_button->set_icon_name("system-lock-screen-symbolic");
        attach(*m_lock_aspect_ratio_button, 3, top - 2, 1, 2);
        m_lock_aspect_ratio_button->set_active(m_cluster.m_lock_aspect_ratio);
        m_lock_aspect_ratio_button->signal_toggled().connect([this] {
            m_cluster.m_lock_aspect_ratio = m_lock_aspect_ratio_button->get_active();
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
    }

private:
    EntityCluster &m_cluster;

    SpinButtonAngle *m_angle_sp = nullptr;
    Gtk::SpinButton *m_scale_x_sp = nullptr;
    Gtk::SpinButton *m_scale_y_sp = nullptr;

    Gtk::ToggleButton *m_lock_angle_button = nullptr;
    Gtk::ToggleButton *m_lock_scale_x_button = nullptr;
    Gtk::ToggleButton *m_lock_scale_y_button = nullptr;
    Gtk::ToggleButton *m_lock_aspect_ratio_button = nullptr;
};

class PictureEditor : public Gtk::Grid, public ChangeableCommitMode {
public:
    PictureEditor(Document &doc, const UUID &cluster) : m_picture(doc.get_entity<EntityPicture>(cluster))
    {
        set_row_spacing(5);
        set_column_spacing(5);
        int top = 0;


        m_angle_sp = Gtk::make_managed<SpinButtonAngle>();
        m_angle_sp->set_hexpand(true);
        m_angle_sp->set_value(m_picture.m_angle);
        connect_spinbutton(*m_angle_sp,
                           [this] { return update_if_changed(m_picture.m_angle, m_angle_sp->get_value()); });
        grid_attach_label_and_widget(*this, "Angle", *m_angle_sp, top);

        m_lock_angle_button = Gtk::make_managed<Gtk::ToggleButton>();
        m_lock_angle_button->set_tooltip_text("Lock angle");
        m_lock_angle_button->set_icon_name("system-lock-screen-symbolic");
        m_lock_angle_button->set_active(m_picture.m_lock_angle);
        m_lock_angle_button->signal_toggled().connect([this] {
            m_picture.m_lock_angle = m_lock_angle_button->get_active();
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
        attach(*m_lock_angle_button, 2, top - 1);
        top++;

        m_lock_aspect_ratio_button = Gtk::make_managed<Gtk::CheckButton>("Lock aspect ratio");
        attach(*m_lock_aspect_ratio_button, 0, top, 2);
        m_lock_aspect_ratio_button->set_active(m_picture.m_lock_aspect_ratio);
        m_lock_aspect_ratio_button->signal_toggled().connect([this] {
            m_picture.m_lock_aspect_ratio = m_lock_aspect_ratio_button->get_active();
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
    }

private:
    EntityPicture &m_picture;

    SpinButtonAngle *m_angle_sp = nullptr;

    Gtk::ToggleButton *m_lock_angle_button = nullptr;
    Gtk::CheckButton *m_lock_aspect_ratio_button = nullptr;
};

class TextEditor : public Gtk::Grid, public ChangeableCommitMode {
public:
    TextEditor(Document &doc, const UUID &text) : m_text(doc.get_entity<EntityText>(text)), m_doc(doc)
    {
        set_row_spacing(5);
        set_column_spacing(5);
        int top = 0;

        m_text_entry = Gtk::make_managed<Gtk::Entry>();
        m_text_entry->set_text(m_text.m_text);
        connect_entry(*m_text_entry, [this] {
            m_text.m_text = m_text_entry->get_text();
            render_text(m_text, get_pango_context(), m_doc);
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
        grid_attach_label_and_widget(*this, "Text", *m_text_entry, top);

        auto dialog = Gtk::FontDialog::create();
        m_font_button = Gtk::make_managed<Gtk::FontDialogButton>(dialog);
        m_font_button->set_level(Gtk::FontLevel::FEATURES);
        m_font_button->set_font_desc(Pango::FontDescription(m_text.m_font));
        m_font_button->property_font_desc().signal_changed().connect([this] {
            m_text.m_font = m_font_button->get_font_desc().to_string();
            render_text(m_text, get_pango_context(), m_doc);
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
        m_font_button->property_font_features().signal_changed().connect([this] {
            m_text.m_font_features = m_font_button->get_font_features();
            render_text(m_text, get_pango_context(), m_doc);
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
        grid_attach_label_and_widget(*this, "Font", *m_font_button, top);

        m_angle_sp = Gtk::make_managed<SpinButtonAngle>();
        m_angle_sp->set_hexpand(true);
        m_angle_sp->set_value(m_text.m_angle);
        connect_spinbutton(*m_angle_sp, [this] { return update_if_changed(m_text.m_angle, m_angle_sp->get_value()); });
        grid_attach_label_and_widget(*this, "Angle", *m_angle_sp, top);

        m_lock_angle_button = Gtk::make_managed<Gtk::ToggleButton>();
        m_lock_angle_button->set_tooltip_text("Lock angle");
        m_lock_angle_button->set_icon_name("system-lock-screen-symbolic");
        m_lock_angle_button->set_active(m_text.m_lock_angle);
        m_lock_angle_button->signal_toggled().connect([this] {
            m_text.m_lock_angle = m_lock_angle_button->get_active();
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
        attach(*m_lock_angle_button, 2, top - 1);

        m_scale_sp = Gtk::make_managed<Gtk::SpinButton>();
        m_scale_sp->set_hexpand(true);
        m_scale_sp->set_range(-1e3, 1e3);
        m_scale_sp->set_increments(.1, .1);
        m_scale_sp->set_digits(4);
        m_scale_sp->set_value(m_text.m_scale);
        connect_spinbutton(*m_scale_sp, [this] { return update_if_changed(m_text.m_scale, m_scale_sp->get_value()); });
        grid_attach_label_and_widget(*this, "Scale", *m_scale_sp, top);

        m_lock_scale_button = Gtk::make_managed<Gtk::ToggleButton>();
        m_lock_scale_button->set_tooltip_text("Lock scale");
        m_lock_scale_button->set_icon_name("system-lock-screen-symbolic");
        m_lock_scale_button->set_active(m_text.m_lock_scale);
        m_lock_scale_button->signal_toggled().connect([this] {
            m_text.m_lock_scale = m_lock_scale_button->get_active();
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
        attach(*m_lock_scale_button, 2, top - 1);
    }

private:
    EntityText &m_text;
    const Document &m_doc;

    Gtk::Entry *m_text_entry = nullptr;
    Gtk::FontDialogButton *m_font_button = nullptr;

    SpinButtonAngle *m_angle_sp = nullptr;
    Gtk::SpinButton *m_scale_sp = nullptr;

    Gtk::ToggleButton *m_lock_angle_button = nullptr;
    Gtk::ToggleButton *m_lock_scale_button = nullptr;
};

void SelectionEditor::set_selection(const std::set<SelectableRef> &sel)
{
    if (m_editor) {
        remove(*m_editor);
        m_editor = nullptr;
    }
    if (m_view_editor) {
        remove(*m_view_editor);
        m_view_editor = nullptr;
    }
    if (m_core.has_documents()) {
        if (auto wrkpl = point_from_selection(m_core.get_current_document(), sel, Entity::Type::WORKPLANE)) {
            m_title->set_label("Workplane");
            m_title->set_tooltip_text((std::string)wrkpl->entity);
            auto ed = Gtk::make_managed<WorkplaneEditor>(m_core.get_current_document(), wrkpl->entity);
            m_editor = ed;
            ed->signal_changed().connect([this](auto mode) { m_signal_changed.emit(mode); });
        }
        else if (auto step = point_from_selection(m_core.get_current_document(), sel, Entity::Type::STEP)) {
            m_title->set_label("STEP");
            m_title->set_tooltip_text((std::string)step->entity);
            auto ed = Gtk::make_managed<STEPEditor>(m_core.get_current_document_directory(),
                                                    m_core.get_current_document().get_entity<EntitySTEP>(step->entity));
            m_editor = ed;
            auto group = m_core.get_current_document().get_entity(step->entity).m_group;
            ed->signal_changed().connect([this, group](auto mode) {
                m_core.get_current_document().set_group_update_solid_model_pending(group);
                m_signal_changed.emit(mode);
            });

            auto ved =
                    Gtk::make_managed<EntityViewEditorSTEP>(m_doc_view_prv.get_current_document_view(), step->entity);
            m_view_editor = ved;
            ved->signal_changed().connect([this] { m_signal_view_changed.emit(); });
        }
        else if (auto doc = point_from_selection(m_core.get_current_document(), sel, Entity::Type::DOCUMENT)) {
            m_title->set_label("Document");
            m_title->set_tooltip_text((std::string)doc->entity);
            auto ed = Gtk::make_managed<DocumentEditor>(
                    m_core.get_current_document_directory(),
                    m_core.get_current_document().get_entity<EntityDocument>(doc->entity));
            m_editor = ed;
            ed->signal_changed().connect([this](auto mode) { m_signal_changed.emit(mode); });
        }
        else if (auto cluster = point_from_selection(m_core.get_current_document(), sel, Entity::Type::CLUSTER)) {
            m_title->set_label("Cluster");
            m_title->set_tooltip_text((std::string)cluster->entity);
            auto ed = Gtk::make_managed<ClusterEditor>(m_core.get_current_document(), cluster->entity);
            m_editor = ed;
            auto group = m_core.get_current_document().get_entity(cluster->entity).m_group;
            ed->signal_changed().connect([this, group](auto mode) {
                m_core.get_current_document().set_group_solve_pending(group);
                m_signal_changed.emit(mode);
            });
        }
        else if (auto picture = point_from_selection(m_core.get_current_document(), sel, Entity::Type::PICTURE)) {
            m_title->set_label("Picture");
            m_title->set_tooltip_text((std::string)picture->entity);
            auto ed = Gtk::make_managed<PictureEditor>(m_core.get_current_document(), picture->entity);
            m_editor = ed;
            auto group = m_core.get_current_document().get_entity(picture->entity).m_group;
            ed->signal_changed().connect([this, group](auto mode) {
                m_core.get_current_document().set_group_solve_pending(group);
                m_signal_changed.emit(mode);
            });
        }
        else if (auto text = point_from_selection(m_core.get_current_document(), sel, Entity::Type::TEXT)) {
            m_title->set_label("Text");
            m_title->set_tooltip_text((std::string)text->entity);
            auto ed = Gtk::make_managed<TextEditor>(m_core.get_current_document(), text->entity);
            m_editor = ed;
            auto group = m_core.get_current_document().get_entity(text->entity).m_group;
            ed->signal_changed().connect([this, group](auto mode) {
                m_core.get_current_document().set_group_solve_pending(group);
                m_signal_changed.emit(mode);
            });
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
    if (m_view_editor) {
        m_view_editor->set_margin(10);
        append(*m_view_editor);
    }
}

} // namespace dune3d

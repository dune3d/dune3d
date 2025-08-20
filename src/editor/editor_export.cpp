#include "editor.hpp"
#include "action/action_id.hpp"
#include "util/util.hpp"
#include "util/fs_util.hpp"
#include "document/group/igroup_solid_model.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/solid_model/solid_model.hpp"
#include "document/export_paths.hpp"
#include "document/export_dxf.hpp"
#include "canvas/canvas.hpp"
#include "dune3d_appwindow.hpp"
#include "dune3d_application.hpp"
#include "util/template_util.hpp"
#include <iostream>

namespace dune3d {

static Glib::RefPtr<Gio::File>
get_export_initial_filename(const Dune3DApplication::UserConfig &cfg, const IDocumentInfo &doc_info,
                            const std::string &suffix,
                            std::string Dune3DApplication::UserConfig::ExportPaths::*export_type)
{
    if (!doc_info.has_path())
        return nullptr;
    auto current_group = doc_info.get_current_group();
    auto groups_sorted = doc_info.get_document().get_groups_sorted();
    std::string filename;
    for (auto it : groups_sorted) {
        const auto k = std::make_pair(doc_info.get_path(), it->m_uuid);
        if (cfg.export_paths.contains(k))
            filename = cfg.export_paths.at(k).*export_type;

        if (it->m_uuid == current_group && filename.size()) {
            if (filename.ends_with(suffix))
                filename = filename.substr(0, filename.size() - suffix.length());
            break;
        }
    }
    if (!filename.size()) {
        static const char *d3ddoc_suffix = ".d3ddoc";
        // fallback to document
        auto doc_fn = path_to_string(doc_info.get_path());
        if (doc_fn.size() && doc_fn.ends_with(d3ddoc_suffix))
            filename = doc_fn.substr(0, doc_fn.size() - strlen(d3ddoc_suffix));
    }

    if (filename.size())
        return Gio::File::create_for_path(filename);
    else
        return nullptr;
}


static void set_export_initial_filename(Dune3DApplication::UserConfig &cfg, const IDocumentInfo &doc_info,
                                        std::string Dune3DApplication::UserConfig::ExportPaths::*export_type,
                                        const std::string &filename)
{
    if (!doc_info.has_path())
        return;
    cfg.export_paths[std::make_pair(doc_info.get_path(), doc_info.get_current_group())].*export_type = filename;
}
void Editor::on_export_solid_model(const ActionConnection &conn)
{
    const auto action = std::get<ActionID>(conn.id);
    auto dialog = Gtk::FileDialog::create();

    using ExPaths = Dune3DApplication::UserConfig::ExportPaths;

    decltype(&ExPaths::step) export_type = nullptr;

    // Add filters, so that only certain file types can be selected:
    auto filters = Gio::ListStore<Gtk::FileFilter>::create();

    auto filter_any = Gtk::FileFilter::create();
    std::string suffix;
    if (action == ActionID::EXPORT_SOLID_MODEL_STEP) {
        filter_any->set_name("STEP");
        filter_any->add_pattern("*.step");
        filter_any->add_pattern("*.stp");
        suffix = ".step";
        export_type = &ExPaths::step;
    }
    else {
        filter_any->set_name("STL");
        filter_any->add_pattern("*.stl");
        suffix = ".stl";
        export_type = &ExPaths::stl;
    }
    filters->append(filter_any);

    if (auto initial_file = get_export_initial_filename(m_win.get_app().m_user_config,
                                                        m_core.get_current_idocument_info(), suffix, export_type)) {
        dialog->set_initial_file(initial_file);
    }

    dialog->set_filters(filters);

    // Show the dialog and wait for a user response:
    dialog->save(m_win, [this, dialog, action, suffix, export_type](const Glib::RefPtr<Gio::AsyncResult> &result) {
        try {
            auto file = dialog->save_finish(result);
            // open_file_view(file);
            //  Notice that this is a std::string, not a Glib::ustring.
            const auto path = path_from_string(append_suffix_if_required(file->get_path(), suffix));
            auto &group = m_core.get_current_document().get_group(m_core.get_current_group());
            if (auto gr = dynamic_cast<const IGroupSolidModel *>(&group)) {
                if (action == ActionID::EXPORT_SOLID_MODEL_STEP)
                    gr->get_solid_model()->export_step(path);
                else
                    gr->get_solid_model()->export_stl(path);
                set_export_initial_filename(m_win.get_app().m_user_config, m_core.get_current_idocument_info(),
                                            export_type, path_to_string(path));
            }
        }
        catch (const Gtk::DialogError &err) {
            // Can be thrown by dialog->open_finish(result).
            std::cout << "No file selected. " << err.what() << std::endl;
        }
        catch (const Glib::Error &err) {
            std::cout << "Unexpected exception. " << err.what() << std::endl;
        }
    });
}

void Editor::on_export_paths(const ActionConnection &conn)
{
    const auto action = std::get<ActionID>(conn.id);

    auto dialog = Gtk::FileDialog::create();

    std::string suffix = ".svg";
    if (action == ActionID::EXPORT_DXF_CURRENT_GROUP)
        suffix = ".dxf";

    if (auto initial_file =
                get_export_initial_filename(m_win.get_app().m_user_config, m_core.get_current_idocument_info(), suffix,
                                            &Dune3DApplication::UserConfig::ExportPaths::paths)) {
        dialog->set_initial_file(initial_file);
    }

    // Add filters, so that only certain file types can be selected:
    auto filters = Gio::ListStore<Gtk::FileFilter>::create();

    auto filter_any = Gtk::FileFilter::create();
    if (action == ActionID::EXPORT_DXF_CURRENT_GROUP) {
        filter_any->set_name("DXF");
        filter_any->add_pattern("*.dxf");
    }
    else {
        filter_any->set_name("SVG");
        filter_any->add_pattern("*.svg");
    }
    filters->append(filter_any);

    dialog->set_filters(filters);


    // Show the dialog and wait for a user response:
    dialog->save(m_win, [this, dialog, action, suffix](const Glib::RefPtr<Gio::AsyncResult> &result) {
        try {
            auto file = dialog->save_finish(result);
            // open_file_view(file);
            //  Notice that this is a std::string, not a Glib::ustring.
            const auto path = path_from_string(append_suffix_if_required(file->get_path(), suffix));

            auto group_filter = [this, action](const Group &group) {
                if (m_core.get_current_group() == group.m_uuid)
                    return true;
                if (any_of(action, ActionID::EXPORT_PATHS_IN_CURRENT_GROUP, ActionID::EXPORT_DXF_CURRENT_GROUP))
                    return false;
                auto &body_group = group.find_body(m_core.get_current_document()).group;
                auto &doc_view = get_current_document_view();
                auto group_visible = doc_view.group_is_visible(group.m_uuid);
                auto body_visible = doc_view.body_is_visible(body_group.m_uuid);
                return body_visible && group_visible;
            };

            if (action == ActionID::EXPORT_DXF_CURRENT_GROUP)
                export_dxf(path, m_core.get_current_document(), m_core.get_current_group());
            else
                export_paths(path, m_core.get_current_document(), m_core.get_current_group(), group_filter);
            set_export_initial_filename(m_win.get_app().m_user_config, m_core.get_current_idocument_info(),
                                        &Dune3DApplication::UserConfig::ExportPaths::paths, path_to_string(path));
        }
        catch (const Gtk::DialogError &err) {
            // Can be thrown by dialog->open_finish(result).
            std::cout << "No file selected. " << err.what() << std::endl;
        }
        catch (const Glib::Error &err) {
            std::cout << "Unexpected exception. " << err.what() << std::endl;
        }
    });
}

void Editor::on_export_projection(const ActionConnection &conn)
{
    const bool all_groups = std::get<ActionID>(conn.id) == ActionID::EXPORT_PROJECTION_ALL;
    auto dialog = Gtk::FileDialog::create();

    if (auto initial_file =
                get_export_initial_filename(m_win.get_app().m_user_config, m_core.get_current_idocument_info(), ".svg",
                                            &Dune3DApplication::UserConfig::ExportPaths::projection)) {
        dialog->set_initial_file(initial_file);
    }

    // Add filters, so that only certain file types can be selected:
    auto filters = Gio::ListStore<Gtk::FileFilter>::create();

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("SVG");
    filter_any->add_pattern("*.svg");
    filters->append(filter_any);

    dialog->set_filters(filters);

    // Show the dialog and wait for a user response:
    dialog->save(m_win, [this, dialog, all_groups](const Glib::RefPtr<Gio::AsyncResult> &result) {
        try {
            auto file = dialog->save_finish(result);
            // open_file_view(file);
            //  Notice that this is a std::string, not a Glib::ustring.
            const auto path = path_from_string(append_suffix_if_required(file->get_path(), ".svg"));

            auto sel = get_canvas().get_selection();
            glm::dvec3 origin = get_canvas().get_center();
            glm::dquat normal = get_canvas().get_cam_quat();
            for (const auto &it : sel) {
                if (it.type == SelectableRef::Type::ENTITY) {
                    if (m_core.get_current_document().m_entities.count(it.item)
                        && m_core.get_current_document().m_entities.at(it.item)->get_type()
                                   == Entity::Type::WORKPLANE) {
                        auto &wrkpl = m_core.get_current_document().get_entity<EntityWorkplane>(it.item);
                        origin = wrkpl.m_origin;
                        normal = wrkpl.m_normal;
                        break;
                    }
                }
            }
            {
                auto &doc = m_core.get_current_document();
                auto &current_group = doc.get_group(m_core.get_current_group());

                if (all_groups) {
                    auto &current_body_group = current_group.find_body(doc).group;
                    auto groups_by_body = doc.get_groups_by_body();
                    std::vector<const SolidModel *> solids;
                    for (auto body_groups : groups_by_body) {
                        if (!get_current_document_view().body_solid_model_is_visible(body_groups.get_group().m_uuid))
                            continue;
                        if (current_body_group.m_uuid != body_groups.get_group().m_uuid
                            && !get_current_document_view().body_is_visible(body_groups.get_group().m_uuid))
                            continue;
                        const SolidModel *last_solid_model = nullptr;
                        for (auto group : body_groups.groups) {
                            if (current_group.m_uuid != group->m_uuid
                                && !get_current_document_view().group_is_visible(group->m_uuid))
                                continue;
                            if (auto gr = dynamic_cast<const IGroupSolidModel *>(group)) {
                                if (gr->get_solid_model()) {
                                    last_solid_model = gr->get_solid_model();
                                }
                                if (group->m_uuid == m_core.get_current_idocument_info().get_current_group())
                                    break;
                            }
                        }
                        if (last_solid_model) {
                            solids.push_back(last_solid_model);
                        }
                    }
                    SolidModel::export_projections(path, solids, origin, normal);
                }
                else {
                    if (auto gr = dynamic_cast<const IGroupSolidModel *>(&current_group); gr && gr->get_solid_model()) {
                        SolidModel::export_projections(path, {gr->get_solid_model()}, origin, normal);
                    }
                }

                set_export_initial_filename(m_win.get_app().m_user_config, m_core.get_current_idocument_info(),
                                            &Dune3DApplication::UserConfig::ExportPaths::projection,
                                            path_to_string(path));
            }
        }
        catch (const Gtk::DialogError &err) {
            // Can be thrown by dialog->open_finish(result).
            std::cout << "No file selected. " << err.what() << std::endl;
        }
        catch (const Glib::Error &err) {
            std::cout << "Unexpected exception. " << err.what() << std::endl;
        }
    });
}
} // namespace dune3d

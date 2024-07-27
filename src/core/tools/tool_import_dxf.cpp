#include "tool_import_dxf.hpp"
#include "document/document.hpp"
#include "document/entity/entity_step.hpp"
#include "editor/editor_interface.hpp"
#include "dialogs/dialogs.hpp"
#include "import_step/step_import_manager.hpp"
#include <gtkmm.h>
#include "util/fs_util.hpp"
#include "tool_common_impl.hpp"
#include "core/tool_data_path.hpp"
#include "import_dxf/dxf_importer.hpp"
#include "canvas/selection_mode.hpp"

namespace dune3d {

ToolResponse ToolImportDXF::begin(const ToolArgs &args)
{
    auto dialog = Gtk::FileDialog::create();
    {
        auto dir = m_core.get_current_document_directory();
        if (!dir.empty())
            dialog->set_initial_folder(Gio::File::create_for_path(path_to_string(dir)));
    }

    // Add filters, so that only certain file types can be selected:
    auto filters = Gio::ListStore<Gtk::FileFilter>::create();

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("DXF");
    filter_any->add_pattern("*.dxf");
    filter_any->add_pattern("*.DXF");
    filters->append(filter_any);

    dialog->set_filters(filters);

    // Show the dialog and wait for a user response:
    dialog->open(m_intf.get_dialogs().get_parent(), [this, dialog](const Glib::RefPtr<Gio::AsyncResult> &result) {
        try {
            auto file = dialog->open_finish(result);
            // Notice that this is a std::string, not a Glib::ustring.
            auto filename = file->get_path();
            m_intf.tool_update_data(std::make_unique<ToolDataPath>(path_from_string(filename)));
        }
        catch (const Gtk::DialogError &err) {
            // Can be thrown by dialog->open_finish(result).
            m_intf.tool_update_data(std::make_unique<ToolDataPath>());
        }
        catch (const Glib::Error &err) {
            m_intf.tool_update_data(std::make_unique<ToolDataPath>());
        }
    });


    return ToolResponse();
}

ToolBase::CanBegin ToolImportDXF::can_begin()
{
    return m_core.get_current_workplane() != UUID{};
}


ToolResponse ToolImportDXF::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataPath *>(args.data.get())) {
            if (data->path != std::filesystem::path{}) {
                DXFImporter importer(get_doc(), m_core.get_current_group(), m_core.get_current_workplane());
                if (!importer.import(data->path))
                    return ToolResponse::revert();
                auto ents = importer.get_entities();
                set_current_group_generate_pending();
                m_intf.set_canvas_selection_mode(SelectionMode::NORMAL);
                m_selection.clear();
                for (auto en : ents) {
                    if (en->of_type(Entity::Type::CIRCLE_2D))
                        m_selection.emplace(SelectableRef::Type::ENTITY, en->m_uuid, 1);
                    else
                        m_selection.emplace(SelectableRef::Type::ENTITY, en->m_uuid, 0);
                }
                return ToolResponse::commit();
            }
            else {
                return ToolResponse::end();
            }
        }
    }
    return ToolResponse();
}
} // namespace dune3d

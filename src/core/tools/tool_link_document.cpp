#include "tool_link_document.hpp"
#include "document/document.hpp"
#include "document/entity/entity_document.hpp"
#include "editor/editor_interface.hpp"
#include "dialogs/dialogs.hpp"
#include "import_step/step_import_manager.hpp"
#include <gtkmm.h>
#include "util/fs_util.hpp"
#include "tool_common_impl.hpp"
#include "core/tool_data_path.hpp"

namespace dune3d {

ToolBase::CanBegin ToolLinkDocument::can_begin()
{
    return can_create_entity();
}

ToolResponse ToolLinkDocument::begin(const ToolArgs &args)
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
    filter_any->set_name("Dune 3D documents");
    filter_any->add_pattern("*.d3ddoc");
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


ToolResponse ToolLinkDocument::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataPath *>(args.data.get())) {
            if (data->path != std::filesystem::path{}) {
                auto &step = add_entity<EntityDocument>();
                auto dir = m_core.get_current_document_directory();
                if (auto rel = get_relative_filename(data->path, dir))
                    step.m_path = *rel;
                else
                    step.m_path = data->path;
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

#include "tool_import_step.hpp"
#include "document/document.hpp"
#include "document/entity/entity_step.hpp"
#include "editor/editor_interface.hpp"
#include "dialogs/dialogs.hpp"
#include "import_step/step_import_manager.hpp"
#include <gtkmm.h>
#include <iostream>
#include "tool_common_impl.hpp"

namespace dune3d {

class ToolDataPath : public ToolData {
public:
    ToolDataPath(const std::filesystem::path &p) : path(p)
    {
    }
    ToolDataPath()
    {
    }
    std::filesystem::path path;
};

ToolResponse ToolImportSTEP::begin(const ToolArgs &args)
{
    auto dialog = Gtk::FileDialog::create();

    // Add filters, so that only certain file types can be selected:
    auto filters = Gio::ListStore<Gtk::FileFilter>::create();

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("STEP");
    filter_any->add_pattern("*.step");
    filter_any->add_pattern("*.stp");
    filters->append(filter_any);

    dialog->set_filters(filters);

    // Show the dialog and wait for a user response:
    dialog->open(m_intf.get_dialogs().get_parent(), [this, dialog](const Glib::RefPtr<Gio::AsyncResult> &result) {
        try {
            auto file = dialog->open_finish(result);
            // Notice that this is a std::string, not a Glib::ustring.
            auto filename = file->get_path();
            m_intf.tool_update_data(std::make_unique<ToolDataPath>(filename));
            std::cout << "File selected: " << filename << std::endl;
        }
        catch (const Gtk::DialogError &err) {
            // Can be thrown by dialog->open_finish(result).
            std::cout << "No file selected. " << err.what() << std::endl;
            m_intf.tool_update_data(std::make_unique<ToolDataPath>());
        }
        catch (const Glib::Error &err) {
            std::cout << "Unexpected exception. " << err.what() << std::endl;
            m_intf.tool_update_data(std::make_unique<ToolDataPath>());
        }
    });


    return ToolResponse();
}


ToolResponse ToolImportSTEP::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataPath *>(args.data.get())) {
            if (data->path != std::filesystem::path{}) {
                auto &step = add_entity<EntitySTEP>();
                step.m_path = data->path;
                step.m_imported = STEPImportManager::get().import_step(step.m_path);
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

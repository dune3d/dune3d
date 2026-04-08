#include "tool_import_step.hpp"
#include "document/document.hpp"
#include "document/entity/entity_step.hpp"
#include "document/constraint/constraint_lock_rotation.hpp"
#include "editor/editor_interface.hpp"
#include "dialogs/dialogs.hpp"
#include "import_step/step_import_manager.hpp"
#include <gtkmm.h>
#include "util/fs_util.hpp"
#include "util/glm_util.hpp"
#include "util/action_label.hpp"
#include "tool_common_impl.hpp"
#include "core/tool_data_path.hpp"

namespace dune3d {

ToolBase::CanBegin ToolImportSTEP::can_begin()
{
    return can_create_entity();
}

ToolResponse ToolImportSTEP::begin(const ToolArgs &args)
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
    filter_any->set_name("STEP");
    filter_any->add_pattern("*.step");
    filter_any->add_pattern("*.STEP");
    filter_any->add_pattern("*.stp");
    filter_any->add_pattern("*.STP");
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


void ToolImportSTEP::update_tip()
{
    if (!m_step)
        return;

    std::string tip = "Orientation: " + quat_to_string(m_step->m_normal);

    std::vector<ConstraintType> constraint_icons;

    if (m_lock_rotation)
        constraint_icons.push_back(ConstraintType::LOCK_ROTATION);

    std::vector<ActionLabelInfo> actions;
    actions.reserve(9);
    actions.emplace_back(InToolActionID::LMB, "place");

    actions.emplace_back(InToolActionID::RMB, "cancel");

    if (m_lock_rotation)
        actions.emplace_back(InToolActionID::TOGGLE_LOCK_ROTATION_CONSTRAINT, "unlock rotation");
    else
        actions.emplace_back(InToolActionID::TOGGLE_LOCK_ROTATION_CONSTRAINT, "lock rotation");

    actions.emplace_back(InToolActionID::ROTATE_X, InToolActionID::ROTATE_Y, InToolActionID::ROTATE_Z, "rotate");

    m_intf.set_constraint_icons(m_intf.get_cursor_pos(), {NAN, NAN, NAN}, constraint_icons);

    m_intf.tool_bar_set_actions(actions);
    m_intf.tool_bar_set_tool_tip(tip);
}

ToolResponse ToolImportSTEP::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE && m_step) {
        m_step->m_origin = m_intf.get_cursor_pos();

        set_first_update_group_current();
    }
    else if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataPath *>(args.data.get())) {
            if (data->path != std::filesystem::path{}) {
                m_step = &add_entity<EntitySTEP>();
                auto dir = m_core.get_current_document_directory();
                if (auto rel = get_relative_filename(data->path, dir))
                    m_step->m_path = *rel;
                else
                    m_step->m_path = data->path;
                m_step->update_imported(dir);
                m_step->m_origin = m_intf.get_cursor_pos();
            }
            else {
                return ToolResponse::end();
            }
        }
    }

    else if (args.type == ToolEventType::ACTION && m_step) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (m_lock_rotation) {
                auto &constraint = add_constraint<ConstraintLockRotation>();
                constraint.m_entity = m_step->m_uuid;
            }
            return ToolResponse::commit();
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        case InToolActionID::TOGGLE_LOCK_ROTATION_CONSTRAINT: {
            m_lock_rotation = !m_lock_rotation;
        } break;

        case InToolActionID::ROTATE_X:
        case InToolActionID::ROTATE_Y:
        case InToolActionID::ROTATE_Z: {
            const auto axis = static_cast<int>(args.action) - static_cast<int>(InToolActionID::ROTATE_X);
            glm::dvec3 a = {0, 0, 0};
            a[axis] = 1;
            const auto q = glm::angleAxis(M_PI / 2, a);

            m_lock_rotation = true;
            m_step->m_normal = q * m_step->m_normal;

        } break;

        default:;
        }
    }

    update_tip();

    return ToolResponse();
}
} // namespace dune3d

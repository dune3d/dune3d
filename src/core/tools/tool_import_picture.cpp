#include "tool_import_picture.hpp"
#include "document/document.hpp"
#include "document/entity/entity_picture.hpp"
#include "document/entity/entity_workplane.hpp"
#include "editor/editor_interface.hpp"
#include "dialogs/dialogs.hpp"
#include <gtkmm.h>
#include "util/fs_util.hpp"
#include "util/picture_util.hpp"
#include "util/picture_data.hpp"
#include "tool_common_impl.hpp"
#include "core/tool_data_path.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include "util/action_label.hpp"

namespace dune3d {

ToolResponse ToolImportPicture::begin(const ToolArgs &args)
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
    filter_any->add_pixbuf_formats();
    filter_any->set_name("Pictures");
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

    m_wrkpl = get_workplane();

    update_tip();


    return ToolResponse();
}

ToolBase::CanBegin ToolImportPicture::can_begin()
{
    return get_workplane_uuid() != UUID{};
}


ToolResponse ToolImportPicture::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataPath *>(args.data.get())) {
            if (data->path != std::filesystem::path{}) {
                auto pic_data = picture_data_from_file(data->path);
                m_pic = &add_entity<EntityPicture>();
                m_pic->m_wrkpl = get_workplane_uuid();
                m_pic->m_origin = m_wrkpl->project(get_cursor_pos_for_workplane(*m_wrkpl));
                m_pic->m_data = pic_data;
                m_pic->m_data_uuid = pic_data->m_uuid;
                m_pic->m_width = pic_data->m_width;
                m_pic->m_height = pic_data->m_height;
                {
                    const auto m = std::max(m_pic->m_width, m_pic->m_height);
                    const double scale = 10. / m;
                    m_pic->m_scale_x = scale;
                    m_pic->m_scale_y = scale;
                }
                m_pic->m_lock_aspect_ratio = true;
                m_pic->m_selection_invisible = true;
                m_pic->update_builtin_anchors();
                m_intf.enable_hover_selection();
            }
            else {
                return ToolResponse::end();
            }
        }
    }
    else if (args.type == ToolEventType::MOVE && m_pic) {
        m_pic->m_origin = m_wrkpl->project(get_cursor_pos_for_workplane(*m_wrkpl));
        set_first_update_group_current();
    }
    else if (args.type == ToolEventType::ACTION && m_pic) {
        switch (args.action) {
        case InToolActionID::LMB:
            m_pic->m_selection_invisible = false;
            if (m_constrain) {
                const EntityAndPoint origin{m_pic->m_uuid, 1};
                constrain_point(get_workplane_uuid(), origin);
            }
            return ToolResponse::commit();

        case InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT:
            m_constrain = !m_constrain;
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
    }
    update_tip();
    return ToolResponse();
}

void ToolImportPicture::update_tip()
{
    std::vector<ActionLabelInfo> actions;

    actions.emplace_back(InToolActionID::LMB, "place");
    actions.emplace_back(InToolActionID::RMB, "cancel");

    if (m_constrain)
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "constraint off");
    else
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "constraint on");


    std::vector<ConstraintType> constraint_icons;
    glm::vec3 v = {NAN, NAN, NAN};

    m_intf.tool_bar_set_tool_tip("");
    if (m_constrain) {
        set_constrain_tip("origin");
        update_constraint_icons(constraint_icons);
    }

    m_intf.set_constraint_icons(get_cursor_pos_for_workplane(*m_wrkpl), v, constraint_icons);

    m_intf.tool_bar_set_actions(actions);
}

} // namespace dune3d

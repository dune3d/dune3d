#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <pangomm.h>
#include "canvas/selectable_ref.hpp"
#include "core/tool_data.hpp"

namespace dune3d {

class ActionLabelInfo;
class Dialogs;
enum class SelectionMode;
enum class ConstraintType;
struct ItemsToDelete;
class Buffer;

class EditorInterface {
public:
    virtual glm::dvec3 get_cursor_pos() const = 0;
    virtual glm::vec3 get_cam_normal() const = 0;
    virtual glm::quat get_cam_quat() const = 0;
    virtual glm::dvec3 get_cursor_pos_for_plane(glm::dvec3 origin, glm::dvec3 normal) const = 0;

    virtual void tool_bar_set_actions(const std::vector<ActionLabelInfo> &labels) = 0;
    virtual void tool_bar_set_tool_tip(const std::string &s) = 0;
    virtual void tool_bar_flash(const std::string &s) = 0;
    virtual void tool_bar_flash_replace(const std::string &s) = 0;

    virtual void tool_update_data(std::unique_ptr<ToolData> data) = 0;
    virtual void enable_hover_selection(bool enable = true) = 0;
    virtual std::optional<SelectableRef> get_hover_selection() const = 0;
    virtual void set_no_canvas_update(bool v) = 0;
    virtual void set_canvas_selection_mode(SelectionMode mode) = 0;
    virtual void canvas_update_from_tool() = 0;
    virtual void set_solid_model_edge_select_mode(bool v) = 0;

    virtual bool get_use_workplane() const = 0;

    virtual void set_constraint_icons(glm::vec3 p, glm::vec3 v, const std::vector<ConstraintType> &constraints) = 0;

    virtual void show_delete_items_popup(const ItemsToDelete &items_selected, const ItemsToDelete &items_all) = 0;

    virtual Dialogs &get_dialogs() = 0;

    virtual Glib::RefPtr<Pango::Context> get_pango_context() = 0;

    virtual void set_buffer(std::unique_ptr<const Buffer> buffer) = 0;
    virtual const Buffer *get_buffer() const = 0;

    virtual void set_first_update_group(const UUID &group) = 0;
};
} // namespace dune3d

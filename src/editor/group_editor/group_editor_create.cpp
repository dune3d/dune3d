#include "group_editor.hpp"
#include "document/group/group.hpp"

#include "group_editor_extrude.hpp"
#include "group_editor_mirror.hpp"
#include "group_editor_loft.hpp"
#include "group_editor_revolve.hpp"
#include "group_editor_fillet.hpp"
#include "group_editor_reference.hpp"
#include "group_editor_array.hpp"
#include "group_editor_solid_model_operation.hpp"
#include "group_editor_pipe.hpp"
#include "group_editor_clone.hpp"
#include "group_editor_exploded_cluster.hpp"
#include "core/core.hpp"

namespace dune3d {

class GroupEditorSketch : public GroupEditorSolidModel {
public:
    GroupEditorSketch(Core &core, const UUID &group_uu) : GroupEditorSolidModel(core, group_uu)
    {
        add_operation_combo();
    }
};


class GroupEditorLathe : public GroupEditorSweep {
public:
    GroupEditorLathe(Core &core, const UUID &group_uu) : GroupEditorSweep(core, group_uu)
    {
        add_operation_combo();
    }
};

GroupEditor *GroupEditor::create(Core &core, const UUID &group_uu)
{
    auto &group = core.get_current_document().get_group(group_uu);
    switch (group.get_type()) {
    case Group::Type::SKETCH:
        return Gtk::make_managed<GroupEditorSketch>(core, group_uu);
    case Group::Type::EXTRUDE:
        return Gtk::make_managed<GroupEditorExtrude>(core, group_uu);
    case Group::Type::LATHE:
        return Gtk::make_managed<GroupEditorLathe>(core, group_uu);
    case Group::Type::REVOLVE:
        return Gtk::make_managed<GroupEditorRevolve>(core, group_uu);
    case Group::Type::FILLET:
    case Group::Type::CHAMFER:
        return Gtk::make_managed<GroupEditorFillet>(core, group_uu);
    case Group::Type::REFERENCE:
        return Gtk::make_managed<GroupEditorReference>(core, group_uu);
    case Group::Type::LINEAR_ARRAY:
    case Group::Type::POLAR_ARRAY:
        return Gtk::make_managed<GroupEditorArray>(core, group_uu);
    case Group::Type::LOFT:
        return Gtk::make_managed<GroupEditorLoft>(core, group_uu);
    case Group::Type::MIRROR_VERTICAL:
    case Group::Type::MIRROR_HORIZONTAL:
        return Gtk::make_managed<GroupEditorMirror>(core, group_uu);
    case Group::Type::EXPLODED_CLUSTER:
        return Gtk::make_managed<GroupEditorExplodedCluster>(core, group_uu);
    case Group::Type::SOLID_MODEL_OPERATION:
        return Gtk::make_managed<GroupEditorSolidModelOperation>(core, group_uu);
    case Group::Type::CLONE:
        return Gtk::make_managed<GroupEditorClone>(core, group_uu);
    case Group::Type::PIPE:
        return Gtk::make_managed<GroupEditorPipe>(core, group_uu);
    default:
        return Gtk::make_managed<GroupEditor>(core, group_uu);
    }
}
} // namespace dune3d

#include "group_editor_chamfer.hpp"

namespace dune3d {

GroupEditorChamfer::GroupEditorChamfer(Core &core, const UUID &group_uu) : GroupEditorLocalOperation(core, group_uu)
{
    construct();
}

} // namespace dune3d

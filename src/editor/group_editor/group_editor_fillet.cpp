#include "group_editor_fillet.hpp"

namespace dune3d {

GroupEditorFillet::GroupEditorFillet(Core &core, const UUID &group_uu) : GroupEditorLocalOperation(core, group_uu)
{
    construct();
}

} // namespace dune3d

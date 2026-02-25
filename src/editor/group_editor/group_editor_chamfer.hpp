#pragma once
#include "group_editor_local_operation.hpp"

namespace dune3d {

class GroupEditorChamfer : public GroupEditorLocalOperation {
public:
    GroupEditorChamfer(Core &core, const UUID &group_uu);
};

} // namespace dune3d

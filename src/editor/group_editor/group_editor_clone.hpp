#pragma once
#include "group_editor.hpp"

namespace dune3d {

class GroupClone;

class GroupEditorClone : public GroupEditor {
public:
    GroupEditorClone(Core &core, const UUID &group_uu);

private:
    GroupClone &get_group();
};

} // namespace dune3d

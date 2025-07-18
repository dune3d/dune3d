#pragma once
#include "group_editor_solid_model.hpp"

namespace dune3d {

class GroupSweep;

class GroupEditorSweep : public GroupEditorSolidModel {
protected:
    using GroupEditorSolidModel::GroupEditorSolidModel;
    void add_operation_combo();

private:
    GroupSweep &get_group();
};

} // namespace dune3d

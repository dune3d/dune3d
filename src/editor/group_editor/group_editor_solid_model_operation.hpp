#pragma once
#include "group_editor_solid_model.hpp"

namespace dune3d {

class GroupSolidModelOperation;
class GroupButton;

class GroupEditorSolidModelOperation : public GroupEditorSolidModel {
public:
    GroupEditorSolidModelOperation(Core &core, const UUID &group_uu);

private:
    GroupButton *m_argument_button = nullptr;
    GroupButton *m_tool_button = nullptr;

    void do_reload() override;

    void changed();
    GroupSolidModelOperation &get_group();
};

} // namespace dune3d

#pragma once
#include "group_editor.hpp"

namespace dune3d {

class IGroupSolidModel;

class GroupEditorSolidModel : public GroupEditor {
protected:
    using GroupEditor::GroupEditor;
    void add_operation_combo();
    void do_reload() override;

private:
    IGroupSolidModel &get_group();

protected:
    Gtk::DropDown *m_operation_combo = nullptr;
};

} // namespace dune3d

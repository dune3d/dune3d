#pragma once
#include "group_editor.hpp"

namespace dune3d {

class GroupArray;

class GroupEditorArray : public GroupEditor {
public:
    GroupEditorArray(Core &core, const UUID &group_uu);

    void do_reload() override;

private:
    GroupArray &get_group();

    bool update_count();
    Gtk::SpinButton *m_sp_count = nullptr;
    Gtk::DropDown *m_offset_combo = nullptr;
};

} // namespace dune3d

#pragma once
#include "group_editor_replicate.hpp"

namespace dune3d {

class GroupArray;

class GroupEditorArray : public GroupEditorReplicate {
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

#pragma once
#include "group_editor.hpp"

namespace dune3d {

class GroupMirror;

class GroupEditorMirror : public GroupEditor {
public:
    GroupEditorMirror(Core &core, const UUID &group_uu);

private:
    void do_reload() override;
    GroupMirror &get_group();

    Gtk::Switch *m_include_source_switch = nullptr;
};

} // namespace dune3d

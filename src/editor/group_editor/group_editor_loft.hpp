#pragma once
#include "group_editor_solid_model.hpp"

namespace dune3d {

class GroupLoft;

class GroupEditorLoft : public GroupEditorSolidModel {
public:
    GroupEditorLoft(Core &core, const UUID &group_uu);

private:
    Gtk::Switch *m_ruled_switch = nullptr;
    Gtk::Button *m_source_groups_button = nullptr;
    Gtk::Label *m_source_groups_button_label = nullptr;

    void do_reload() override;

    void update_source_groups_label(const GroupLoft &group);

    void edit_source_groups();

    GroupLoft &get_group();
};
} // namespace dune3d

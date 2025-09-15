#pragma once
#include "group_editor_solid_model.hpp"

namespace dune3d {

class GroupReplicate;
class GroupButton;

class GroupEditorReplicate : public GroupEditorSolidModel {
protected:
    using GroupEditorSolidModel::GroupEditorSolidModel;
    void add_source_widgets();

    void do_reload() override;

private:
    GroupReplicate &get_group();
    Gtk::DropDown *m_sources_combo = nullptr;
    GroupButton *m_source_group_button = nullptr;
    GroupButton *m_source_group_start_button = nullptr;
    Gtk::Label *m_ellipsis_label = nullptr;

    void update_replicate();
    void changed();
};

} // namespace dune3d

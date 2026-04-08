#pragma once
#include "group_editor_local_operation.hpp"

namespace dune3d {

class GroupChamfer;

class GroupEditorChamfer : public GroupEditorLocalOperation {
public:
    GroupEditorChamfer(Core &core, const UUID &group_uu);

    void do_reload() override;

protected:
    void construct_extra() override;
    std::string get_radius_label() const override;
    Gtk::Widget &pack_radius_sp(Gtk::Widget &w) override;

private:
    GroupChamfer &get_group();

    bool update_radius2();

    SpinButtonDim *m_radius2_sp = nullptr;
    Gtk::CheckButton *m_radius2_cb = nullptr;
};

} // namespace dune3d

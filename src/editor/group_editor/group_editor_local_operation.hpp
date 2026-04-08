#pragma once
#include "group_editor.hpp"

namespace dune3d {

class GroupLocalOperation;
class SpinButtonDim;

class GroupEditorLocalOperation : public GroupEditor {
public:
    using GroupEditor::GroupEditor;

    void do_reload() override;

protected:
    void construct();
    virtual void construct_extra();
    virtual std::string get_radius_label() const;
    virtual Gtk::Widget &pack_radius_sp(Gtk::Widget &w);
    Glib::RefPtr<Gtk::SizeGroup> m_cb_sg;

private:
    GroupLocalOperation &get_group();

    bool update_radius();

    SpinButtonDim *m_radius_sp = nullptr;
};


} // namespace dune3d

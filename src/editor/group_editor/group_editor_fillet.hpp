#pragma once
#include "group_editor.hpp"

namespace dune3d {

class GroupLocalOperation;
class SpinButtonDim;

class GroupEditorFillet : public GroupEditor {
public:
    GroupEditorFillet(Core &core, const UUID &group_uu);

    void do_reload() override;

private:
    GroupLocalOperation &get_group();

    bool update_radius();

    SpinButtonDim *m_radius_sp = nullptr;
};

} // namespace dune3d

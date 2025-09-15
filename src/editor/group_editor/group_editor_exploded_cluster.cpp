#include "group_editor_exploded_cluster.hpp"
#include "action/action_id.hpp"

namespace dune3d {

GroupEditorExplodedCluster::GroupEditorExplodedCluster(Core &core, const UUID &group_uu) : GroupEditor(core, group_uu)
{
    auto button = Gtk::make_managed<Gtk::Button>("Unexplode cluster");
    button->signal_clicked().connect([this] { m_signal_trigger_action.emit(ActionID::UNEXPLODE_CLUSTER); });
    attach(*button, 0, m_top++, 2, 1);
}

} // namespace dune3d

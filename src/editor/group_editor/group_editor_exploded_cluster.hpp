#pragma once
#include "group_editor.hpp"

namespace dune3d {

class GroupEditorExplodedCluster : public GroupEditor {
public:
    GroupEditorExplodedCluster(Core &core, const UUID &group_uu);
};

} // namespace dune3d

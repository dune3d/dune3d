#pragma once
#include "canvas/iselection_menu_creator.hpp"

namespace dune3d {

class Core;

class SelectionMenuCreator : public ISelectionMenuCreator {
public:
    SelectionMenuCreator(Core &core);
    std::vector<SelectableCheckButton *> create(Gtk::Popover &popover,
                                                const SelectableRefAndVertexTypeList &list) override;

private:
    Core &m_core;
};
} // namespace dune3d

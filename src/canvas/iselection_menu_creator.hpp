#pragma once
#include <vector>
#include <optional>
#include "selectable_ref.hpp"
#include "icanvas.hpp"
#include <gtkmm.h>

namespace dune3d {

class SelectableCheckButton;

class ISelectionMenuCreator {
public:
    struct SelectableRefAndVertexType {
        ICanvas::VertexType vertex_type;
        std::optional<SelectableRef> selectable;
    };
    using SelectableRefAndVertexTypeList = std::vector<SelectableRefAndVertexType>;

    virtual std::vector<SelectableCheckButton *> create(Gtk::Popover &popover,
                                                        const SelectableRefAndVertexTypeList &list) = 0;
};
} // namespace dune3d

#pragma once

namespace dune3d {

class SelectableRef;

class ISelectionFilter {
public:
    virtual bool can_select(const SelectableRef &sr) const = 0;
};
} // namespace dune3d

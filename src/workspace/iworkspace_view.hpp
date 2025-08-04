#pragma once

namespace dune3d {

class IWorkspaceView {
public:
    virtual bool construction_entities_from_previous_groups_are_visible() const = 0;
    virtual bool hide_irrelevant_workplanes() const = 0;
    virtual float get_curvature_comb_scale() const = 0;
};

} // namespace dune3d

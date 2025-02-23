#include "tool_common.hpp"

namespace dune3d {

class EntityPicture;

class ToolMovePictureAnchor : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool is_specific() override
    {
        return true;
    }

    CanBegin can_begin() override;

private:
    EntityPicture *m_picture = nullptr;
    EntityWorkplane *m_wrkpl = nullptr;
    unsigned int m_anchor;
};

} // namespace dune3d

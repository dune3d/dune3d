#pragma once
#include "tool_data.hpp"
#include <filesystem>

namespace dune3d {
class ToolDataPath : public ToolData {
public:
    ToolDataPath(const std::filesystem::path &p) : path(p)
    {
    }
    ToolDataPath()
    {
    }
    std::filesystem::path path;
};
} // namespace dune3d

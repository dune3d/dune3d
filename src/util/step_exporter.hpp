#pragma once
#include "color.hpp"
#include <filesystem>

class TopoDS_Shape;

namespace dune3d {
class STEPExporter {
public:
    STEPExporter(const char *assy_name);
    ~STEPExporter();
    void add_model(const char *name, const TopoDS_Shape &shape, const Color &color);
    void write(const std::filesystem::path &path) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace dune3d

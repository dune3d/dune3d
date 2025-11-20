#pragma once
#include "color.hpp"
#include <TopoDS.hxx>
#include <TDocStd_Document.hxx>
#include <filesystem>

namespace dune3d {
class STEPExporter {
public:
    STEPExporter();
    void add_component(const char *name, const TopoDS_Shape &shape, const Color &color);
    void write(const std::filesystem::path &path) const;

private:
    Handle(TDocStd_Document) m_doc;
    TDF_Label m_assy;
};
} // namespace dune3d

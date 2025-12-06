#include "step_exporter.hpp"
#include "util/fs_util.hpp"

#include <APIHeaderSection_MakeHeader.hxx>
#include <Quantity_Color.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <Standard_Version.hxx>
#include <TDataStd_Name.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>

#include <memory>

namespace dune3d {
struct STEPExporter::Impl {
    Handle(TDocStd_Document) doc;
    TDF_Label assy;
};

STEPExporter::STEPExporter(const char *assy_name) : m_impl(std::make_unique<Impl>())
{
    auto app = XCAFApp_Application::GetApplication();
    app->NewDocument("MDTV-XCAF", m_impl->doc);
    auto shape_tool = XCAFDoc_DocumentTool::ShapeTool(m_impl->doc->Main());
    m_impl->assy = shape_tool->NewShape();
    if (assy_name)
        TDataStd_Name::Set(m_impl->assy, assy_name);
}

STEPExporter::~STEPExporter() = default;

void STEPExporter::add_model(const char *name, const TopoDS_Shape &shape, const Color &color)
{
    auto shape_tool = XCAFDoc_DocumentTool::ShapeTool(m_impl->doc->Main());
    TDF_Label label = shape_tool->AddShape(shape, false);
    if (name)
        TDataStd_Name::Set(label, name);
    shape_tool->AddComponent(m_impl->assy, label, shape.Location());

    auto color_tool = XCAFDoc_DocumentTool::ColorTool(m_impl->doc->Main());
    auto occ_color = Quantity_Color{color.r, color.g, color.b, Quantity_TOC_sRGB};
    color_tool->SetColor(label, occ_color, XCAFDoc_ColorGen);
}

void STEPExporter::write(const std::filesystem::path &path) const
{
#if OCC_VERSION_MAJOR >= 7 && OCC_VERSION_MINOR >= 2
    auto shape_tool = XCAFDoc_DocumentTool::ShapeTool(m_impl->doc->Main());
    shape_tool->UpdateAssemblies();
#endif

    STEPCAFControl_Writer writer;
    writer.SetColorMode(Standard_True);
    writer.SetNameMode(Standard_True);
    if (Standard_False == writer.Transfer(m_impl->doc, STEPControl_AsIs)) {
        throw std::runtime_error("transfer error");
    }

    APIHeaderSection_MakeHeader hdr(writer.ChangeWriter().Model());
    hdr.SetName(new TCollection_HAsciiString("Body"));
    hdr.SetAuthorValue(1, new TCollection_HAsciiString("An Author"));
    hdr.SetOrganizationValue(1, new TCollection_HAsciiString("A Company"));
    hdr.SetOriginatingSystem(new TCollection_HAsciiString("Dune 3D"));
    hdr.SetDescriptionValue(1, new TCollection_HAsciiString("Body"));

    if (Standard_False == writer.Write(path_to_string(path).c_str()))
        throw std::runtime_error("write error");
}

} // namespace dune3d

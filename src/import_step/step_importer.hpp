#pragma once
#include "import.hpp"
#include <TDocStd_Document.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <Quantity_Color.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <glm/glm.hpp>
#include <filesystem>

namespace dune3d::STEPImporter {
class STEPImporter {
public:
    STEPImporter(const std::filesystem::path &filename);

    Result get_faces_and_points();
    bool is_loaded() const
    {
        return loaded;
    }
    std::vector<TopoDS_Shape> get_shapes();

private:
    bool readSTEP(const char *fname);
    bool processNode(const TopoDS_Shape &shape);
    bool processComp(const TopoDS_Shape &shape, const glm::dmat4 &mat_in = glm::dmat4(1));
    bool processSolid(const TopoDS_Shape &shape, const glm::dmat4 &mat_in = glm::dmat4(1));
    bool getColor(TDF_Label label, Quantity_Color &color);
    bool processShell(const TopoDS_Shape &shape, Quantity_Color *color, const glm::dmat4 &mat = glm::dmat4(1));
    bool processFace(const TopoDS_Face &face, Quantity_Color *color, const glm::dmat4 &mat = glm::dmat4(1));
    void processWire(const TopoDS_Wire &wire, const glm::dmat4 &mat);

    Handle(XCAFApp_Application) m_app;
    Handle(TDocStd_Document) m_doc;
    Handle(XCAFDoc_ColorTool) m_color;
    Handle(XCAFDoc_ShapeTool) m_assy;
    bool hasSolid;
    bool loaded = false;

    Result *result;
};
} // namespace dune3d::STEPImporter

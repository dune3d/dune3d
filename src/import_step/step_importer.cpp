#include "step_importer.hpp"
#include <Quantity_Color.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFApp_Application.hxx>
#include <Standard_Version.hxx>

#include <AIS_Shape.hxx>

#include <IGESCAFControl_Reader.hxx>
#include <IGESControl_Reader.hxx>
#include <Interface_Static.hxx>

#include <STEPCAFControl_Reader.hxx>
#include <STEPControl_Reader.hxx>

#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>

#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

#include <Poly_PolygonOnTriangulation.hxx>
#include <Poly_Triangulation.hxx>
#include <TShort_Array1OfShortReal.hxx>
#include <Precision.hxx>
#include <Quantity_Color.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <ShapeAnalysis_Edge.hxx>
#include <BRepAdaptor_Curve.hxx>

#include <TDF_ChildIterator.hxx>
#include <TDF_LabelSequence.hxx>
#include <Poly.hxx>
#include <gp_Circ.hxx>
#include <GCPnts_TangentialDeflection.hxx>

#define USER_PREC (0.14)
#define USER_ANGLE (0.52359878)

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>
#include <map>
#include <list>

#include "util/fs_util.hpp"

namespace dune3d::STEPImporter {

// adapted from
// https://github.com/KiCad/kicad-source-mirror/blob/master/plugins/3d/oce/loadmodel.cpp


bool STEPImporter::readSTEP(const char *fname)
{
    STEPCAFControl_Reader reader;
    IFSelect_ReturnStatus stat = reader.ReadFile(fname);

    if (stat != IFSelect_RetDone)
        return false;

    // Enable user-defined shape precision
    if (!Interface_Static::SetIVal("read.precision.mode", 1))
        return false;

    // Set the shape conversion precision to USER_PREC (default 0.0001 has too
    // many triangles)
    if (!Interface_Static::SetRVal("read.precision.val", USER_PREC))
        return false;

    // set other translation options
    reader.SetColorMode(true);  // use model colors
    reader.SetNameMode(false);  // don't use label names
    reader.SetLayerMode(false); // ignore LAYER data

    if (!reader.Transfer(m_doc)) {
        m_doc->Close();
        return false;
    }

    // are there any shapes to translate?
    if (reader.NbRootsForTransfer() < 1)
        return false;

    return true;
}


STEPImporter::STEPImporter(const std::filesystem::path &filename)
{
    m_app = XCAFApp_Application::GetApplication();

    m_app->NewDocument("MDTV-XCAF", m_doc);
    if (!readSTEP(path_to_string(filename).c_str())) {
        std::cout << "error loading " << filename << std::endl;
        loaded = false;
        return;
    }
    std::cout << "loaded" << std::endl;
    loaded = true;

    m_assy = XCAFDoc_DocumentTool::ShapeTool(m_doc->Main());
    m_color = XCAFDoc_DocumentTool::ColorTool(m_doc->Main());
}

void STEPImporter::processWire(const TopoDS_Wire &wire, const glm::dmat4 &mat)
{
    for (BRepTools_WireExplorer expl(wire); expl.More(); expl.Next()) {
        const auto &edge = expl.Current();
        ShapeAnalysis_Edge sh_an;
        auto first = sh_an.FirstVertex(edge);
        {
            gp_Pnt pnt = BRep_Tool::Pnt(first);
            glm::dvec4 gpnt(pnt.X(), pnt.Y(), pnt.Z(), 1);
            auto pt = mat * gpnt;
            result->points.emplace_back(pt.x, pt.y, pt.z);
        }

        auto curve = BRepAdaptor_Curve(edge);
        const auto curvetype = curve.GetType();
        if (curvetype == GeomAbs_Circle) {
            gp_Circ c = curve.Circle();
            auto pnt = c.Location();
            {
                glm::dvec4 gpnt(pnt.X(), pnt.Y(), pnt.Z(), 1);
                auto pt = mat * gpnt;
                result->points.emplace_back(pt.x, pt.y, pt.z);
            }
        }
    }
}

#if OCC_VERSION_MAJOR >= 7 && OCC_VERSION_MINOR >= 6
#define HORIZON_NEW_OCC
#endif

bool STEPImporter::processFace(const TopoDS_Face &face, Quantity_Color *color, const glm::dmat4 &mat)
{
    if (Standard_True == face.IsNull())
        return false;

    {
        TopoDS_Iterator it;
        for (it.Initialize(face, false, false); it.More(); it.Next()) {
            const TopoDS_Shape &subShape = it.Value();
            TopAbs_ShapeEnum stype = subShape.ShapeType();
            if (stype == TopAbs_WIRE) {
                const TopoDS_Wire &wire = TopoDS::Wire(it.Value());
                processWire(wire, mat);
            }
        }
    }

    // bool reverse = ( face.Orientation() == TopAbs_REVERSED );

    //	bool useBothSides = false;

    TopLoc_Location loc;
    Standard_Boolean isTessellate(Standard_False);
    Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, loc);

    if (triangulation.IsNull() || triangulation->Deflection() > USER_PREC + Precision::Confusion())
        isTessellate = Standard_True;

    if (isTessellate) {
        BRepMesh_IncrementalMesh IM(face, USER_PREC, Standard_False, USER_ANGLE);
        triangulation = BRep_Tool::Triangulation(face, loc);
    }

    if (triangulation.IsNull() == Standard_True)
        return false;

    Quantity_Color lcolor;

    // check for a face color; this has precedence over SOLID colors
    do {
        TDF_Label L;

        if (m_color->ShapeTool()->Search(face, L)) {
            if (m_color->GetColor(L, XCAFDoc_ColorGen, lcolor) || m_color->GetColor(L, XCAFDoc_ColorCurv, lcolor)
                || m_color->GetColor(L, XCAFDoc_ColorSurf, lcolor))
                color = &lcolor;
        }
    } while (0);

    Poly::ComputeNormals(triangulation);

#ifndef HORIZON_NEW_OCC
    const TColgp_Array1OfPnt &arrPolyNodes = triangulation->Nodes();
    const Poly_Array1OfTriangle &arrTriangles = triangulation->Triangles();
    const TShort_Array1OfShortReal &arrNormals = triangulation->Normals();
#endif


    result->faces.emplace_back();
    auto &face_out = result->faces.back();
    if (color) {
        face_out.color.r = color->Red();
        face_out.color.g = color->Green();
        face_out.color.b = color->Blue();
    }
    else {
        face_out.color = {0.5, 0.5, 0.5};
    }
    face_out.vertices.reserve(triangulation->NbNodes());

    std::map<Vertex, std::vector<size_t>> pts_map;
    for (int i = 1; i <= triangulation->NbNodes(); i++) {
#ifdef HORIZON_NEW_OCC
        gp_XYZ v(triangulation->Node(i).Coord());
#else
        gp_XYZ v(arrPolyNodes(i).Coord());
#endif
        const glm::vec4 vg(v.X(), v.Y(), v.Z(), 1);
        const auto vt = mat * vg;
        const Vertex vertex(vt.x, vt.y, vt.z);
        pts_map[vertex].push_back(i - 1);
        face_out.vertices.push_back(vertex);
    }

    face_out.normals.reserve(triangulation->NbNodes());
    for (int i = 1; i <= triangulation->NbNodes(); i++) {
#ifdef HORIZON_NEW_OCC
        const auto n = triangulation->Normal(i);
        glm::vec4 vg(n.X(), n.Y(), n.Z(), 0);
#else
        auto offset = (i - 1) * 3 + 1;
        auto x = arrNormals(offset + 0);
        auto y = arrNormals(offset + 1);
        auto z = arrNormals(offset + 2);
        glm::vec4 vg(x, y, z, 0);
#endif
        auto vt = mat * vg;
        vt /= vt.length();
        face_out.normals.emplace_back(vt.x, vt.y, vt.z);
    }

    // average normals at coincident vertices
    for (const auto &[k, v] : pts_map) {
        if (v.size() > 1) {
            Vertex n_acc(0, 0, 0);
            for (const auto idx : v) {
                n_acc += face_out.normals.at(idx);
            }
            n_acc /= v.size();
            for (const auto idx : v) {
                face_out.normals.at(idx) = n_acc;
            }
        }
    }

    face_out.triangle_indices.reserve(triangulation->NbTriangles());
    for (int i = 1; i <= triangulation->NbTriangles(); i++) {
        int a, b, c;
#ifdef HORIZON_NEW_OCC
        triangulation->Triangle(i).Get(a, b, c);
#else
        arrTriangles(i).Get(a, b, c);
#endif
        face_out.triangle_indices.emplace_back(a - 1, b - 1, c - 1);
    }

    return true;
}

bool STEPImporter::processShell(const TopoDS_Shape &shape, Quantity_Color *color, const glm::dmat4 &mat)
{
    TopoDS_Iterator it;
    bool ret = false;

    for (it.Initialize(shape, false, false); it.More(); it.Next()) {
        const TopoDS_Face &face = TopoDS::Face(it.Value());

        if (processFace(face, color, mat))
            ret = true;
    }

    return ret;
}

bool STEPImporter::getColor(TDF_Label label, Quantity_Color &color)
{
    while (true) {
        if (m_color->GetColor(label, XCAFDoc_ColorGen, color))
            return true;
        else if (m_color->GetColor(label, XCAFDoc_ColorSurf, color))
            return true;
        else if (m_color->GetColor(label, XCAFDoc_ColorCurv, color))
            return true;

        label = label.Father();

        if (label.IsNull())
            break;
    };

    return false;
}

bool STEPImporter::processSolid(const TopoDS_Shape &shape, const glm::dmat4 &mat_in)
{
    TDF_Label label = m_assy->FindShape(shape, Standard_False);
    bool ret = false;

    hasSolid = true;

    Quantity_Color col;
    Quantity_Color *lcolor = NULL;

    if (label.IsNull()) {
    }
    else {
        if (getColor(label, col))
            lcolor = &col;
    }

    TopLoc_Location loc = shape.Location();
    gp_Trsf T = loc.Transformation();
    gp_XYZ coord = T.TranslationPart();

    auto mat = mat_in * glm::translate(glm::dvec3(coord.X(), coord.Y(), coord.Z()));

    gp_XYZ axis;
    Standard_Real angle;

    if (T.GetRotation(axis, angle)) {
        glm::dvec3 gaxis(axis.X(), axis.Y(), axis.Z());
        double angle_f = angle;
        mat = glm::rotate(mat, angle_f, gaxis);
    }

    TopoDS_Iterator it;
    for (it.Initialize(shape, false, false); it.More(); it.Next()) {
        const TopoDS_Shape &subShape = it.Value();

        if (processShell(subShape, lcolor, mat))
            ret = true;
    }

    return ret;
}


bool STEPImporter::processComp(const TopoDS_Shape &shape, const glm::dmat4 &mat_in)
{
    TopoDS_Iterator it;
    TopLoc_Location loc = shape.Location();
    gp_Trsf T = loc.Transformation();
    gp_XYZ coord = T.TranslationPart();

    auto mat = mat_in * glm::translate(glm::dvec3(coord.X(), coord.Y(), coord.Z()));

    gp_XYZ axis;
    Standard_Real angle;

    if (T.GetRotation(axis, angle)) {
        glm::dvec3 gaxis(axis.X(), axis.Y(), axis.Z());
        double angle_f = angle;
        mat = glm::rotate(mat, angle_f, gaxis);
    }

    bool ret = false;

    for (it.Initialize(shape, false, false); it.More(); it.Next()) {
        const TopoDS_Shape &subShape = it.Value();
        TopAbs_ShapeEnum stype = subShape.ShapeType();

        hasSolid = false;

        switch (stype) {
        case TopAbs_COMPOUND:
        case TopAbs_COMPSOLID:
            if (processComp(subShape, mat))
                ret = true;
            break;

        case TopAbs_SOLID:
            if (processSolid(subShape, mat))
                ret = true;
            break;

        case TopAbs_SHELL:
            if (processShell(subShape, NULL, mat))
                ret = true;
            break;

        case TopAbs_FACE:
            if (processFace(TopoDS::Face(subShape), NULL, mat))
                ret = true;
            break;

        default:
            break;
        }
    }

    return ret;
}

bool STEPImporter::processNode(const TopoDS_Shape &shape)
{
    TopAbs_ShapeEnum stype = shape.ShapeType();
    bool ret = true;
    switch (stype) {
    case TopAbs_COMPOUND:
    case TopAbs_COMPSOLID:
        if (processComp(shape))
            ret = true;
        break;

    case TopAbs_SOLID:
        if (processSolid(shape))
            ret = true;
        break;

    case TopAbs_SHELL:
        if (processShell(shape, NULL))
            ret = true;
        break;

    case TopAbs_FACE:
        if (processFace(TopoDS::Face(shape), NULL))
            ret = true;
        break;

    default:
        break;
    }

    return ret;
}

Result STEPImporter::get_faces_and_points()
{
    Result res;
    result = &res;

    TDF_LabelSequence frshapes;
    m_assy->GetFreeShapes(frshapes);

    int nshapes = frshapes.Length();
    int id = 1;
    std::cout << "shapes " << nshapes << std::endl;
    while (id <= nshapes) {
        TopoDS_Shape shape = m_assy->GetShape(frshapes.Value(id));
        if (!shape.IsNull() && processNode(shape)) {
            TopExp_Explorer topex(shape, TopAbs_EDGE);
            std::list<TopoDS_Shape> edges;
            while (topex.More()) {
                auto edge = TopoDS::Edge(topex.Current());
                bool skip = false;
                for (const auto &other : edges) {
                    if (other.IsSame(topex.Current())) {
                        skip = true;
                        break;
                    }
                }
                if (skip) {
                    topex.Next();
                    continue;
                }
                edges.push_back(edge);
                {
                    auto curve = BRepAdaptor_Curve(edge);
                    GCPnts_TangentialDeflection discretizer(curve, M_PI / 16, 1e3);
                    auto &e = res.edges.emplace_back();
                    if (discretizer.NbPoints() > 0) {
                        int nbPoints = discretizer.NbPoints();
                        for (int i = 1; i <= nbPoints; i++) {
                            const gp_Pnt pnt = discretizer.Value(i);
                            e.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
                        }
                    }
                }
                topex.Next();
            }
        }
        ++id;
    }

    result = nullptr;
    return res;
}

std::vector<TopoDS_Shape> STEPImporter::get_shapes()
{
    std::vector<TopoDS_Shape> r;
    TDF_LabelSequence frshapes;
    m_assy->GetFreeShapes(frshapes);

    int nshapes = frshapes.Length();
    int id = 1;
    std::cout << "shapes " << nshapes << std::endl;
    while (id <= nshapes) {
        TopoDS_Shape shape = m_assy->GetShape(frshapes.Value(id));
        if (!shape.IsNull()) {
            r.push_back(shape);
        }
        ++id;
    }
    return r;
}

Result import(const std::filesystem::path &filename)
{
    STEPImporter importer(filename);
    if (!importer.is_loaded())
        return {};
    return importer.get_faces_and_points();
}

} // namespace dune3d::STEPImporter

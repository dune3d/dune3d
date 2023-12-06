#include "solid_model_occ.hpp"
#include "preferences/preferences.hpp"
#include "canvas/color_palette.hpp"
#include "util/fs_util.hpp"

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
#include <glm/glm.hpp>
#include <map>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>

#include <StlAPI_Writer.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <APIHeaderSection_MakeHeader.hxx>


#include <GCPnts_TangentialDeflection.hxx>
#include <TDataStd_Name.hxx>

namespace dune3d {

class Triangulator {
public:
    Triangulator(const TopoDS_Shape &shape, face::Faces &faces);


private:
    bool processNode(const TopoDS_Shape &shape);
    bool processComp(const TopoDS_Shape &shape, const glm::dmat4 &mat_in = glm::dmat4(1));
    bool processSolid(const TopoDS_Shape &shape, const glm::dmat4 &mat_in = glm::dmat4(1));
    bool getColor(TDF_Label label, Quantity_Color &color);
    bool processShell(const TopoDS_Shape &shape, Quantity_Color *color, const glm::dmat4 &mat = glm::dmat4(1));
    bool processFace(const TopoDS_Face &face, Quantity_Color *color, const glm::dmat4 &mat = glm::dmat4(1));

    Handle(XCAFApp_Application) m_app;
    Handle(TDocStd_Document) m_doc;
    Handle(XCAFDoc_ColorTool) m_color;
    Handle(XCAFDoc_ShapeTool) m_assy;

    face::Faces &m_faces;
    face::Color m_default_color;
};

Triangulator::Triangulator(const TopoDS_Shape &shape, face::Faces &faces) : m_faces(faces)
{
    {
        auto color = Preferences::get().canvas.appearance.get_color(ColorP::SOLID_MODEL);
        m_default_color.r = color.r;
        m_default_color.b = color.b;
        m_default_color.g = color.g;
    }
    processNode(shape);
}

#if OCC_VERSION_MAJOR >= 7 && OCC_VERSION_MINOR >= 6
#define HORIZON_NEW_OCC
#endif

#define USER_PREC (0.14)
#define USER_ANGLE (0.52359878)

static glm::dmat4 update_matrix(const gp_Trsf &tr, const glm::dmat4 &mat_in)
{
    gp_XYZ coord = tr.TranslationPart();

    auto mat = mat_in * glm::translate(glm::dvec3(coord.X(), coord.Y(), coord.Z()));

    gp_XYZ axis;
    Standard_Real angle;

    if (tr.GetRotation(axis, angle)) {
        glm::dvec3 gaxis(axis.X(), axis.Y(), axis.Z());
        double angle_f = angle;
        mat = glm::rotate(mat, angle_f, gaxis);
    }

    return mat;
}


bool Triangulator::processFace(const TopoDS_Face &face, Quantity_Color *color, const glm::dmat4 &mat_in)
{
    if (Standard_True == face.IsNull())
        return false;

    auto mat = update_matrix(face.Location().Transformation(), mat_in);

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

        if (m_color && m_color->ShapeTool()->Search(face, L)) {
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


    m_faces.emplace_back();
    auto &face_out = m_faces.back();
    if (color) {
        face_out.color.r = color->Red();
        face_out.color.g = color->Green();
        face_out.color.b = color->Blue();
    }
    else {
        face_out.color = m_default_color;
    }
    face_out.vertices.reserve(triangulation->NbNodes());

    std::map<face::Vertex, std::vector<size_t>> pts_map;
    for (int i = 1; i <= triangulation->NbNodes(); i++) {
#ifdef HORIZON_NEW_OCC
        gp_XYZ v(triangulation->Node(i).Coord());
#else
        gp_XYZ v(arrPolyNodes(i).Coord());
#endif
        const glm::vec4 vg(v.X(), v.Y(), v.Z(), 1);
        const auto vt = mat * vg;
        const face::Vertex vertex(vt.x, vt.y, vt.z);
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
            face::Vertex n_acc(0, 0, 0);
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
        // std::cout << "tr " << a - 1 << " " << b - 1 << " " << c - 1 << std::endl;
    }

    return true;
}

bool Triangulator::processShell(const TopoDS_Shape &shape, Quantity_Color *color, const glm::dmat4 &mat)
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

bool Triangulator::getColor(TDF_Label label, Quantity_Color &color)
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

bool Triangulator::processSolid(const TopoDS_Shape &shape, const glm::dmat4 &mat_in)
{
    TDF_Label label;
    if (m_assy)
        label = m_assy->FindShape(shape, Standard_False);
    bool ret = false;

    Quantity_Color col;
    Quantity_Color *lcolor = NULL;

    if (label.IsNull()) {
    }
    else {
        if (getColor(label, col))
            lcolor = &col;
    }

    auto mat = update_matrix(shape.Location().Transformation(), mat_in);

    TopoDS_Iterator it;
    for (it.Initialize(shape, false, false); it.More(); it.Next()) {
        const TopoDS_Shape &subShape = it.Value();

        if (processShell(subShape, lcolor, mat))
            ret = true;
    }

    return ret;
}


bool Triangulator::processComp(const TopoDS_Shape &shape, const glm::dmat4 &mat_in)
{
    TopoDS_Iterator it;

    auto mat = update_matrix(shape.Location().Transformation(), mat_in);

    bool ret = false;

    for (it.Initialize(shape, false, false); it.More(); it.Next()) {
        const TopoDS_Shape &subShape = it.Value();
        TopAbs_ShapeEnum stype = subShape.ShapeType();

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
            if (processShell(subShape, NULL))
                ret = true;
            break;

        case TopAbs_FACE:
            if (processFace(TopoDS::Face(subShape), NULL))
                ret = true;
            break;

        default:
            break;
        }
    }

    return ret;
}

bool Triangulator::processNode(const TopoDS_Shape &shape)
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


void SolidModelOcc::triangulate()
{
    m_faces.clear();
    Triangulator tri{m_shape_acc, m_faces};
}

inline double defaultAngularDeflection(double linearTolerance)
{
    // Default OCC angular deflection is 0.5 radians, or about 28.6 degrees.
    // That is a bit coarser than necessary for performance, so we default to at
    // most 0.1 radians, or 5.7 degrees. We also do not go finer than 0.005, or
    // roughly 0.28 degree angular resolution, to avoid performance tanking
    // completely at very fine resolutions.
    return std::min(0.1, linearTolerance * 5 + 0.005);
}

void SolidModelOcc::export_stl(const std::filesystem::path &path) const
{
    StlAPI_Writer writer;
    double deflection = 0.001;
#if OCC_VERSION_HEX < 0x060801
    if (deflection > 0) {
        writer.RelativeMode() = false;
        writer.SetDeflection(deflection);
    }
#else
    TopoDS_Shape sh = m_shape_acc;
    BRepMesh_IncrementalMesh aMesh(m_shape_acc, deflection,
                                   /*isRelative*/ Standard_False,
                                   /*theAngDeflection*/
                                   0.5,
                                   /*isInParallel*/ true);
#endif
    writer.Write(m_shape_acc, path_to_string(path).c_str());
}


void SolidModelOcc::export_step(const std::filesystem::path &path) const
{
    auto app = XCAFApp_Application::GetApplication();
    Handle(TDocStd_Document) doc;
    app->NewDocument("MDTV-XCAF", doc);

    auto assy = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    auto assy_label = assy->NewShape();
    TDataStd_Name::Set(assy_label, ("PCA"));


    TDF_Label board_label = assy->AddShape(m_shape_acc, false);
    assy->AddComponent(assy_label, board_label, m_shape_acc.Location());
    TDataStd_Name::Set(board_label, "PCB");

#if OCC_VERSION_MAJOR >= 7 && OCC_VERSION_MINOR >= 2
    assy->UpdateAssemblies();
#endif

    STEPCAFControl_Writer writer;
    writer.SetColorMode(Standard_True);
    writer.SetNameMode(Standard_True);
    if (Standard_False == writer.Transfer(doc, STEPControl_AsIs)) {
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

void SolidModelOcc::find_edges()
{
    m_edges.clear();
    TopExp_Explorer topex(m_shape_acc, TopAbs_EDGE);
    std::list<TopoDS_Shape> edges;
    unsigned int edge_idx = 0;
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
            edge_idx++;
            continue;
        }
        edges.push_back(edge);
        {
            auto curve = BRepAdaptor_Curve(edge);
            GCPnts_TangentialDeflection discretizer(curve, M_PI / 16, 1e3);
            auto &e = m_edges[edge_idx];
            if (discretizer.NbPoints() > 0) {
                int nbPoints = discretizer.NbPoints();
                for (int i = 1; i <= nbPoints; i++) {
                    const gp_Pnt pnt = discretizer.Value(i);
                    e.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
                }
            }
        }
        topex.Next();
        edge_idx++;
    }
}

} // namespace dune3d

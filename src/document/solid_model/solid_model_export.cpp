#include "solid_model.hpp"
#include "solid_model_occ.hpp"
#include "util/fs_util.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <TopoDS_Iterator.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_TangentialDeflection.hxx>


#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>


namespace dune3d {

static void process_shapes(const TopoDS_Shape &sh, Cairo::RefPtr<Cairo::Context> ctx)
{
    if (sh.IsNull())
        return;
    assert(sh.ShapeType() == TopAbs_COMPOUND);
    TopoDS_Iterator it;
    for (it.Initialize(sh, false, false); it.More(); it.Next()) {
        const TopoDS_Shape &subShape = it.Value();
        TopAbs_ShapeEnum stype = subShape.ShapeType();
        assert(stype == TopAbs_EDGE);
        auto edge = TopoDS::Edge(subShape);
        auto curve = BRepAdaptor_Curve(edge);
        auto curvetype = curve.GetType();
        if (curvetype == GeomAbs_Circle) {
            const gp_Circ c = curve.Circle();
            auto pos = c.Position();
            const auto loc = c.Location();
            auto a0 = curve.FirstParameter();
            auto a1 = curve.LastParameter();
            auto ao = atan2(pos.XDirection().Y(), pos.XDirection().X());
            const auto z = pos.Direction().Z();
            if (z < 0) {
                a0 = -a0;
                a1 = -a1;
            }
            a0 += ao;
            a1 += ao;
            if (z > 0)
                ctx->arc(loc.X(), loc.Y(), c.Radius(), a0, a1);
            else
                ctx->arc_negative(loc.X(), loc.Y(), c.Radius(), a0, a1);
            ctx->stroke();
        }
        else {
            GCPnts_TangentialDeflection discretizer(curve, M_PI / 16, 1e3);
            if (discretizer.NbPoints() > 0) {
                int nbPoints = discretizer.NbPoints();
                for (int i = 2; i <= nbPoints; i++) {
                    const gp_Pnt pnt1 = discretizer.Value(i - 1);
                    const gp_Pnt pnt2 = discretizer.Value(i);
                    ctx->move_to(pnt1.X(), pnt1.Y());
                    ctx->line_to(pnt2.X(), pnt2.Y());
                    ctx->stroke();
                }
            }
        }
    }
}

void SolidModel::export_projections(const std::filesystem::path &path, std::vector<const SolidModel *> models,
                                    const glm::dvec3 &origin, const glm::dquat &normal)
{
    auto surf = Cairo::RecordingSurface::create();

    {
        auto ctx = Cairo::Context::create(surf);
        ctx->set_source_rgb(0, 0, 0);
        ctx->set_line_width(0.1);
        ctx->scale(1, -1);
        ctx->set_line_cap(Cairo::Context::LineCap::ROUND);

        {
            Handle(HLRBRep_Algo) brep_hlr = new HLRBRep_Algo;
            for (auto model : models) {
                brep_hlr->Add(dynamic_cast<const SolidModelOcc &>(*model).m_shape_acc);
            }

            const auto vo = origin;
            const auto vn = glm::rotate(normal, glm::dvec3(0, 0, 1));
            const auto vx = glm::rotate(normal, glm::dvec3(1, 0, 0));

            gp_Ax2 axis(gp_Pnt(vo.x, vo.y, vo.z), gp_Dir(vn.x, vn.y, vn.z), gp_Dir(vx.x, vx.y, vx.z));
            gp_Trsf transform;

            transform.SetTransformation(axis);

            HLRAlgo_Projector projector(transform, Standard_False, 0);
            /* reverse above can result in a scale factor of -1, which is ignored
             * by default...  but the rest of the matrix is still applied...
             */
            projector.Scaled(Standard_True);

            brep_hlr->Projector(projector);
            brep_hlr->Update();
            brep_hlr->Hide();

            HLRBRep_HLRToShape shapes(brep_hlr);

            process_shapes(shapes.VCompound(), ctx);
            process_shapes(shapes.Rg1LineVCompound(), ctx);
            process_shapes(shapes.OutLineVCompound(), ctx);
        }
    }
    auto extents = surf->ink_extents();

    auto isurf = Cairo::SvgSurface::create(path_to_string(path), extents.width, extents.height);
    cairo_svg_surface_set_document_unit(isurf->cobj(), CAIRO_SVG_UNIT_MM);
    {
        auto icr = Cairo::Context::create(isurf);
        icr->set_source(surf, -extents.x, -extents.y);
        icr->paint();
    }
}


} // namespace dune3d

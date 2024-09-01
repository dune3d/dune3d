#include "solid_model.hpp"
#include "solid_model_util.hpp"
#include "solid_model_occ.hpp"
#include "group/group_sketch.hpp"
#include "document/document.hpp"
#include "document/entity/entity_step.hpp"
#include "import_step/imported_step.hpp"
#include "import_step/shapes.hpp"

#include <BRepBuilderAPI_Transform.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Quaternion.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>

namespace dune3d {

std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, GroupSketch &group)
{
    auto mod = std::make_shared<SolidModelOcc>();

    TopoDS_Shape fused;
    for (const auto &[uu, en] : doc.m_entities) {
        if (en->m_group != group.m_uuid)
            continue;
        if (const auto step = dynamic_cast<const EntitySTEP *>(en.get())) {
            if (!step->m_include_in_solid_model)
                continue;

            if (auto shapes = step->m_imported->get_shapes()) {
                gp_Trsf trsf;
                const auto &org = step->m_origin;
                const auto &norm = step->m_normal;
                trsf.SetTranslation(gp_Vec(org.x, org.y, org.z));
                {
                    gp_Trsf trsf2;
                    trsf2.SetRotation(gp_Quaternion(norm.x, norm.y, norm.z, norm.w));
                    trsf.Multiply(trsf2);
                }

                for (const auto &shape : shapes->shapes) {
                    BRepBuilderAPI_Transform tr{shape, trsf, Standard_True};
                    TopExp_Explorer topex(tr.Shape(), TopAbs_SOLID);
                    while (topex.More()) {
                        if (fused.IsNull())
                            fused = topex.Current();
                        else
                            fused = BRepAlgoAPI_Fuse(fused, topex.Current());
                        topex.Next();
                    }
                }
            }
        }
    }

    if (fused.IsNull())
        return nullptr;

    mod->m_shape = fused;

    if (!mod->update_acc_finish(doc, group))
        return nullptr;

    return mod;
}

} // namespace dune3d

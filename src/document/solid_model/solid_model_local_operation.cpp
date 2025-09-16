#include "solid_model.hpp"
#include "solid_model_occ.hpp"
#include "solid_model_util.hpp"
#include "document/document.hpp"
#include "document/group/group_fillet.hpp"
#include "document/group/group_chamfer.hpp"

#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>

#include <BRep_Tool.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Line.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Edge.hxx>

#include <TopExp_Explorer.hxx>

namespace dune3d {

template <typename T>
std::shared_ptr<const SolidModel> create_local_operation(const Document &doc, GroupLocalOperation &group)
{
    group.m_local_operation_messages.clear();
    if (group.m_entities.size() == 0) {
        group.m_local_operation_messages.emplace_back(GroupStatusMessage::Status::ERR, "no edges");
        return nullptr;
    }

    auto mod = std::make_shared<SolidModelOcc>();

    const auto last_solid_model_group = SolidModel::get_last_solid_model_group(doc, group);
    if (!last_solid_model_group) {
        group.m_local_operation_messages.emplace_back(GroupStatusMessage::Status::ERR, "no solid model group");
        return nullptr;
    }
    group.m_operation = last_solid_model_group->get_operation();
    const auto last_solid_model = dynamic_cast<const SolidModelOcc *>(last_solid_model_group->get_solid_model());
    if (!last_solid_model) {
        group.m_local_operation_messages.emplace_back(GroupStatusMessage::Status::ERR, "no solid model");
        return nullptr;
    }

    std::map<UUID, const Entity *> fillet_entities;

    for (const auto &uu : group.m_entities) {
        fillet_entities.emplace(uu, &doc.get_entity(uu));
    }

    try {
        T mf(last_solid_model->m_shape_acc);
        {
            TopExp_Explorer topex(last_solid_model->m_shape_acc, TopAbs_EDGE);
            std::list<TopoDS_Shape> edges;
            unsigned int edge_idx = 0;
            while (topex.More()) {
                auto edge = TopoDS::Edge(topex.Current());
                
                for (const auto &entity : fillet_entities) {
                    if (solid_model_util::isEntityPartnerToEdge(edge, entity.second, doc)) {
                        mf.Add(group.m_radius, edge);
                    }
                }
                
                topex.Next();
                edge_idx++;
            }
        }


        mf.Build();
        if (!mf.IsDone()) {
            group.m_local_operation_messages.emplace_back(GroupStatusMessage::Status::ERR, "not done");
            return nullptr;
        }

        mod->m_shape_acc = mf.Shape();
    }
    catch (const Standard_Failure &e) {
        std::ostringstream os;
        e.Print(os);
        group.m_local_operation_messages.emplace_back(GroupStatusMessage::Status::ERR, "exception: " + os.str());
    }
    catch (const std::exception &e) {
        group.m_local_operation_messages.emplace_back(GroupStatusMessage::Status::ERR,
                                                      std::string{"exception: "} + e.what());
    }
    catch (...) {
        group.m_local_operation_messages.emplace_back(GroupStatusMessage::Status::ERR, "unknown exception");
    }
    if (mod->m_shape_acc.IsNull()) {
        group.m_local_operation_messages.emplace_back(GroupStatusMessage::Status::ERR, "didn't generate a shape");
        return nullptr;
    }

    mod->finish(doc, group);

    return mod;
}

std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, GroupFillet &group)
{
    return create_local_operation<BRepFilletAPI_MakeFillet>(doc, group);
}

std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, GroupChamfer &group)
{
    return create_local_operation<BRepFilletAPI_MakeChamfer>(doc, group);
}

} // namespace dune3d

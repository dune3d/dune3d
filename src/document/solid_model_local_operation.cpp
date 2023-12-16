#include "solid_model.hpp"
#include "solid_model_occ.hpp"
#include "document.hpp"
#include "group/group_fillet.hpp"
#include "group/group_chamfer.hpp"

#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>

#include <TopExp_Explorer.hxx>

namespace dune3d {

std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, GroupFillet &group)
{
    group.m_local_operation_messages.clear();
    if (group.m_edges.size() == 0) {
        group.m_local_operation_messages.emplace_back(GroupStatusMessage::Status::ERR, "no edges");
        return nullptr;
    }

    auto mod = std::make_shared<SolidModelOcc>();

    const auto last_solid_model_group = get_last_solid_model_group(doc, group);
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

    try {
        BRepFilletAPI_MakeFillet mf(last_solid_model->m_shape_acc);
        {
            TopExp_Explorer topex(last_solid_model->m_shape_acc, TopAbs_EDGE);
            std::list<TopoDS_Shape> edges;
            unsigned int edge_idx = 0;
            while (topex.More()) {
                auto edge = TopoDS::Edge(topex.Current());

                if (group.m_edges.contains(edge_idx)) {
                    mf.Add(group.m_radius, edge);
                }

                topex.Next();
                edge_idx++;
            }
        }


        mf.Build();
        if (!mf.IsDone())
            return nullptr;

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
    if (mod->m_shape_acc.IsNull())
        return nullptr;

    mod->find_edges();
    mod->triangulate();
    return mod;
}

std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, GroupChamfer &group)
{
    group.m_local_operation_messages.clear();
    if (group.m_edges.size() == 0) {
        group.m_local_operation_messages.emplace_back(GroupStatusMessage::Status::ERR, "no edges");
        return nullptr;
    }

    auto mod = std::make_shared<SolidModelOcc>();

    const auto last_solid_model_group = get_last_solid_model_group(doc, group);
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
    try {
        BRepFilletAPI_MakeChamfer mf(last_solid_model->m_shape_acc);
        {
            TopExp_Explorer topex(last_solid_model->m_shape_acc, TopAbs_EDGE);
            std::list<TopoDS_Shape> edges;
            unsigned int edge_idx = 0;
            while (topex.More()) {
                auto edge = TopoDS::Edge(topex.Current());

                if (group.m_edges.contains(edge_idx)) {
                    mf.Add(group.m_radius, edge);
                }

                topex.Next();
                edge_idx++;
            }
        }


        mf.Build();
        if (!mf.IsDone())
            return nullptr;

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
    if (mod->m_shape_acc.IsNull())
        return nullptr;

    mod->find_edges();
    mod->triangulate();
    return mod;
}

} // namespace dune3d

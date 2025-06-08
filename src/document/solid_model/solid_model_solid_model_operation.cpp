#include "solid_model.hpp"
#include "solid_model_occ.hpp"
#include "document/document.hpp"
#include "document/group/group_solid_model_operation.hpp"

#include <TopExp_Explorer.hxx>

namespace dune3d {

std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, GroupSolidModelOperation &group)
{
    auto &groups = doc.get_groups();
    group.m_solid_model_messages.clear();
    if (!groups.contains(group.m_source_group_argument)) {
        group.m_solid_model_messages.emplace_back(GroupStatusMessage::Status::ERR, "no argument source group");
        return nullptr;
    }
    if (!groups.contains(group.m_source_group_tool)) {
        group.m_solid_model_messages.emplace_back(GroupStatusMessage::Status::ERR, "no tool source group");
        return nullptr;
    }

    const auto source_group_argument =
            dynamic_cast<const IGroupSolidModel *>(groups.at(group.m_source_group_argument).get());
    if (!source_group_argument || !source_group_argument->get_solid_model()) {
        group.m_solid_model_messages.emplace_back(GroupStatusMessage::Status::ERR,
                                                  "argument source group has no solid model");
        return nullptr;
    }

    const auto source_group_tool = dynamic_cast<const IGroupSolidModel *>(groups.at(group.m_source_group_tool).get());
    if (!source_group_tool || !source_group_tool->get_solid_model()) {
        group.m_solid_model_messages.emplace_back(GroupStatusMessage::Status::ERR,
                                                  "tool source group has no solid model");
        return nullptr;
    }

    auto &solid_model_argument = dynamic_cast<const SolidModelOcc &>(*source_group_argument->get_solid_model());
    auto &solid_model_tool = dynamic_cast<const SolidModelOcc &>(*source_group_tool->get_solid_model());
    if (solid_model_argument.m_shape_acc.IsNull()) {
        group.m_solid_model_messages.emplace_back(GroupStatusMessage::Status::ERR,
                                                  "argument source group has no shape");
        return nullptr;
    }
    if (solid_model_tool.m_shape_acc.IsNull()) {
        group.m_solid_model_messages.emplace_back(GroupStatusMessage::Status::ERR, "tool source group has no shape");
        return nullptr;
    }

    auto mod = std::make_shared<SolidModelOcc>();

    mod->m_shape_acc =
            SolidModelOcc::calc(group.m_operation, solid_model_argument.m_shape_acc, solid_model_tool.m_shape_acc);

    if (mod->m_shape_acc.IsNull()) {
        group.m_solid_model_messages.emplace_back(GroupStatusMessage::Status::ERR, "didn't generate a shape");
        return nullptr;
    }

    mod->finish(doc, group);

    return mod;
}

} // namespace dune3d

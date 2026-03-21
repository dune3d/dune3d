#include "ipc_server.hpp"
#include "ipc_session.hpp"
#include "editor/editor.hpp"
#include "core/core.hpp"
#include "core/tool.hpp"
#include "document/document.hpp"
#include "document/group/group.hpp"
#include "document/group/igroup_solid_model.hpp"
#include "document/group/group_extrude.hpp"
#include "document/group/group_local_operation.hpp"
#include "document/group/group_fillet.hpp"
#include "document/group/group_chamfer.hpp"
#include "document/group/group_linear_array.hpp"
#include "document/group/group_polar_array.hpp"
#include "document/group/group_revolve.hpp"
#include "document/group/group_loft.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_line3d.hpp"
#include "document/entity/entity_circle2d.hpp"
#include "document/entity/entity_circle3d.hpp"
#include "document/entity/entity_arc2d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/constraint/constraint.hpp"
#include "document/constraint/iconstraint_datum.hpp"
#include "document/solid_model/solid_model.hpp"
#include "util/step_exporter.hpp"
#include "core/tool_id.hpp"
#include "action/action_id.hpp"
#include "action/action_catalog.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include "in_tool_action/in_tool_action_catalog.hpp"
#include "logger/logger.hpp"
#include <glib-unix.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>

namespace dune3d {

IpcServer::IpcServer(Editor &editor) : m_editor(editor)
{
    register_methods();
    setup_socket();
    connect_signals();
}

IpcServer::~IpcServer()
{
    cleanup_socket();
}

void IpcServer::setup_socket()
{
    const char *runtime_dir = std::getenv("XDG_RUNTIME_DIR");
    if (!runtime_dir)
        runtime_dir = std::getenv("TMPDIR");
    if (!runtime_dir)
        runtime_dir = "/tmp";

    m_socket_path = std::filesystem::path(runtime_dir) / ("dune3d-" + std::to_string(getpid()) + ".sock");

    std::filesystem::remove(m_socket_path);

    m_listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_listen_fd < 0) {
        Logger::log_warning("IPC: failed to create socket: " + std::string(std::strerror(errno)),
                            Logger::Domain::EDITOR);
        return;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    auto path_str = m_socket_path.string();
    if (path_str.size() >= sizeof(addr.sun_path)) {
        Logger::log_warning("IPC: socket path too long: " + path_str, Logger::Domain::EDITOR);
        close(m_listen_fd);
        m_listen_fd = -1;
        return;
    }
    strncpy(addr.sun_path, path_str.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(m_listen_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
        Logger::log_warning("IPC: bind failed: " + std::string(std::strerror(errno)), Logger::Domain::EDITOR);
        close(m_listen_fd);
        m_listen_fd = -1;
        return;
    }

    if (listen(m_listen_fd, 4) < 0) {
        Logger::log_warning("IPC: listen failed: " + std::string(std::strerror(errno)), Logger::Domain::EDITOR);
        close(m_listen_fd);
        m_listen_fd = -1;
        std::filesystem::remove(m_socket_path);
        return;
    }

    m_accept_source_id = g_unix_fd_add(
            m_listen_fd, G_IO_IN,
            [](int fd, GIOCondition cond, gpointer data) -> gboolean {
                return static_cast<IpcServer *>(data)->on_accept(fd, cond) ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
            },
            this);

    Logger::log_info("IPC server listening on " + path_str, Logger::Domain::EDITOR);
}

void IpcServer::cleanup_socket()
{
    m_sessions.clear();

    if (m_accept_source_id) {
        g_source_remove(m_accept_source_id);
        m_accept_source_id = 0;
    }

    if (m_listen_fd >= 0) {
        close(m_listen_fd);
        m_listen_fd = -1;
    }

    if (!m_socket_path.empty()) {
        std::filesystem::remove(m_socket_path);
        m_socket_path.clear();
    }
}

bool IpcServer::on_accept(int fd, unsigned int condition)
{
    int client_fd = accept(m_listen_fd, nullptr, nullptr);
    if (client_fd < 0) {
        Logger::log_warning("IPC: accept failed: " + std::string(std::strerror(errno)), Logger::Domain::EDITOR);
        return true;
    }

    m_sessions.push_back(std::make_unique<IpcSession>(*this, client_fd));
    return true;
}

void IpcServer::remove_session(IpcSession *session)
{
    m_sessions.remove_if([session](const auto &s) { return s.get() == session; });
}

json IpcServer::dispatch(const std::string &method, const json &params, IpcSession &session)
{
    auto it = m_methods.find(method);
    if (it == m_methods.end()) {
        throw std::runtime_error("Method not found: " + method);
    }
    return it->second(params, session);
}

void IpcServer::broadcast_notification(const std::string &method, const json &params)
{
    json notification = {{"jsonrpc", "2.0"}, {"method", method}, {"params", params}};
    auto msg = notification.dump() + "\n";

    for (auto &session : m_sessions) {
        if (session->m_subscriptions.empty() || session->m_subscriptions.count(method)) {
            session->send(msg);
        }
    }
}

void IpcServer::connect_signals()
{
    auto &core = m_editor.get_core();

    core.signal_documents_changed().connect([this] { broadcast_notification("event.documents_changed"); });

    core.signal_tool_changed().connect([this] {
        auto &core = m_editor.get_core();
        json params;
        if (core.tool_is_active()) {
            params["tool_active"] = true;
            params["tool_id"] = tool_lut.lookup_reverse(core.get_tool_id());
        }
        else {
            params["tool_active"] = false;
        }
        broadcast_notification("event.tool_changed", params);
    });

    core.signal_rebuilt().connect([this] { broadcast_notification("event.rebuilt"); });

    core.signal_needs_save().connect([this] { broadcast_notification("event.needs_save"); });
}

static double json_to_double(const json &v)
{
    if (v.is_number())
        return v.get<double>();
    if (v.is_string())
        return std::stod(v.get<std::string>());
    throw std::invalid_argument("expected a number");
}

static unsigned int json_to_uint(const json &v)
{
    if (v.is_number())
        return v.get<unsigned int>();
    if (v.is_string())
        return static_cast<unsigned int>(std::stoul(v.get<std::string>()));
    throw std::invalid_argument("expected a number");
}

static std::set<SelectableRef> parse_selection(const json &params)
{
    std::set<SelectableRef> sel;
    if (!params.contains("selection"))
        return sel;

    for (auto &item : params["selection"]) {
        SelectableRef sr;
        auto type_str = item.value("type", "entity");
        if (type_str == "entity")
            sr.type = SelectableRef::Type::ENTITY;
        else if (type_str == "constraint")
            sr.type = SelectableRef::Type::CONSTRAINT;
        else if (type_str == "solid_model_edge")
            sr.type = SelectableRef::Type::SOLID_MODEL_EDGE;
        else
            sr.type = SelectableRef::Type::ENTITY;

        sr.item = UUID(item["uuid"].get<std::string>());
        sr.point = item.value("point", 0u);
        sel.insert(sr);
    }
    return sel;
}

void IpcServer::register_methods()
{
    // ===== General =====

    m_methods["ping"] = [](const json &params, IpcSession &) -> json { return "pong"; };

    m_methods["get_version"] = [](const json &params, IpcSession &) -> json { return {{"protocol", 1}}; };

    // ===== Document queries =====

    m_methods["document.list"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        json docs = json::array();
        for (auto *info : core.get_documents()) {
            docs.push_back({
                    {"uuid", static_cast<std::string>(info->get_uuid())},
                    {"name", info->get_name()},
                    {"path", info->get_path().string()},
                    {"needs_save", info->get_needs_save()},
                    {"read_only", info->is_read_only()},
            });
        }
        return docs;
    };

    m_methods["document.get_current"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.has_documents())
            return nullptr;
        auto &info = core.get_current_idocument_info();
        return {
                {"uuid", static_cast<std::string>(info.get_uuid())},
                {"name", info.get_name()},
                {"path", info.get_path().string()},
                {"needs_save", info.get_needs_save()},
                {"read_only", info.is_read_only()},
        };
    };

    m_methods["document.serialize"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.has_documents())
            throw std::runtime_error("No document open");
        return core.get_current_document().serialize();
    };

    // ===== Group queries =====

    m_methods["group.list"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.has_documents())
            throw std::runtime_error("No document open");
        auto &doc = core.get_current_document();
        json groups = json::array();
        for (auto *group : doc.get_groups_sorted()) {
            groups.push_back({
                    {"uuid", static_cast<std::string>(group->m_uuid)},
                    {"name", group->m_name},
                    {"type", group->get_type_name()},
            });
        }
        return groups;
    };

    m_methods["group.get_current"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.has_documents())
            throw std::runtime_error("No document open");
        return static_cast<std::string>(core.get_current_group());
    };

    m_methods["group.set_current"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.has_documents())
            throw std::runtime_error("No document open");
        if (!params.contains("uuid"))
            throw std::invalid_argument("missing 'uuid' parameter");
        UUID uu(params["uuid"].get<std::string>());
        core.set_current_group(uu);
        return {{"success", true}};
    };

    m_methods["group.set_property"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.has_documents())
            throw std::runtime_error("No document open");
        if (!params.contains("group"))
            throw std::invalid_argument("missing 'group' parameter");
        if (!params.contains("property"))
            throw std::invalid_argument("missing 'property' parameter");
        if (!params.contains("value"))
            throw std::invalid_argument("missing 'value' parameter");

        UUID group_uu(params["group"].get<std::string>());
        auto property = params["property"].get<std::string>();
        auto &doc = core.get_current_document();
        auto &group = doc.get_group(group_uu);

        if (property == "name") {
            group.m_name = params["value"].get<std::string>();
        }
        else if (property == "operation") {
            auto *solid_group = dynamic_cast<IGroupSolidModel *>(&group);
            if (!solid_group)
                throw std::runtime_error("Group does not support operation property");
            auto val = params["value"].get<std::string>();
            IGroupSolidModel::Operation op;
            if (val == "union")
                op = IGroupSolidModel::Operation::UNION;
            else if (val == "difference")
                op = IGroupSolidModel::Operation::DIFFERENCE;
            else if (val == "intersection")
                op = IGroupSolidModel::Operation::INTERSECTION;
            else
                throw std::invalid_argument("Invalid operation: " + val + " (use union/difference/intersection)");
            solid_group->set_operation(op);
            doc.set_group_update_solid_model_pending(group_uu);
        }
        else if (property == "direction") {
            auto *extrude = dynamic_cast<GroupExtrude *>(&group);
            if (!extrude)
                throw std::runtime_error("Property 'direction' only applies to extrude groups");
            auto val = params["value"].get<std::string>();
            if (val == "normal")
                extrude->m_direction = GroupExtrude::Direction::NORMAL;
            else if (val == "arbitrary")
                extrude->m_direction = GroupExtrude::Direction::ARBITRARY;
            else
                throw std::invalid_argument("Invalid direction: " + val);
            doc.set_group_solve_pending(group_uu);
        }
        else if (property == "mode") {
            auto *extrude = dynamic_cast<GroupExtrude *>(&group);
            if (!extrude)
                throw std::runtime_error("Property 'mode' only applies to extrude groups");
            auto val = params["value"].get<std::string>();
            if (val == "single")
                extrude->m_mode = GroupExtrude::Mode::SINGLE;
            else if (val == "offset")
                extrude->m_mode = GroupExtrude::Mode::OFFSET;
            else if (val == "offset_symmetric")
                extrude->m_mode = GroupExtrude::Mode::OFFSET_SYMMETRIC;
            else
                throw std::invalid_argument("Invalid mode: " + val);
            doc.set_group_generate_pending(group_uu);
        }
        else if (property == "offset_mul") {
            auto *extrude = dynamic_cast<GroupExtrude *>(&group);
            if (!extrude)
                throw std::runtime_error("Property 'offset_mul' only applies to extrude groups");
            extrude->m_offset_mul = json_to_double(params["value"]);
            doc.set_group_solve_pending(group_uu);
        }
        else if (property == "dvec") {
            auto *extrude = dynamic_cast<GroupExtrude *>(&group);
            if (!extrude)
                throw std::runtime_error("Property 'dvec' only applies to extrude groups");
            auto &v = params["value"];
            if (!v.is_array() || v.size() != 3)
                throw std::invalid_argument("dvec must be array of 3 numbers");
            extrude->m_dvec = {v[0].get<double>(), v[1].get<double>(), v[2].get<double>()};
            doc.set_group_solve_pending(group_uu);
        }
        else if (property == "radius") {
            auto *fillet = dynamic_cast<GroupLocalOperation *>(&group);
            if (!fillet)
                throw std::runtime_error("Property 'radius' only applies to fillet/chamfer groups");
            fillet->m_radius = json_to_double(params["value"]);
            doc.set_group_update_solid_model_pending(group_uu);
        }
        else if (property == "radius2") {
            auto *chamfer = dynamic_cast<GroupChamfer *>(&group);
            if (!chamfer)
                throw std::runtime_error("Property 'radius2' only applies to chamfer groups");
            chamfer->m_radius2 = json_to_double(params["value"]);
            doc.set_group_update_solid_model_pending(group_uu);
        }
        else if (property == "count") {
            auto *lin_array = dynamic_cast<GroupLinearArray *>(&group);
            auto *pol_array = dynamic_cast<GroupPolarArray *>(&group);
            if (lin_array) {
                lin_array->m_count = json_to_uint(params["value"]);
                doc.set_group_generate_pending(group_uu);
            }
            else if (pol_array) {
                pol_array->m_count = json_to_uint(params["value"]);
                doc.set_group_generate_pending(group_uu);
            }
            else {
                throw std::runtime_error("Property 'count' only applies to array groups");
            }
        }
        else if (property == "angle") {
            auto *revolve = dynamic_cast<GroupRevolve *>(&group);
            if (!revolve)
                throw std::runtime_error("Property 'angle' only applies to revolve groups");
            revolve->m_angle = json_to_double(params["value"]);
            doc.set_group_solve_pending(group_uu);
        }
        else if (property == "ruled") {
            auto *loft = dynamic_cast<GroupLoft *>(&group);
            if (!loft)
                throw std::runtime_error("Property 'ruled' only applies to loft groups");
            loft->m_ruled = params["value"].get<bool>();
            doc.set_group_update_solid_model_pending(group_uu);
        }
        else {
            throw std::invalid_argument("Unknown property: " + property);
        }

        core.rebuild("ipc group property changed");
        core.set_needs_save();
        m_editor.ipc_canvas_update();

        return {{"success", true}};
    };

    // ===== Entity queries =====

    m_methods["entity.list"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.has_documents())
            throw std::runtime_error("No document open");
        auto &doc = core.get_current_document();

        UUID group_filter;
        if (params.contains("group"))
            group_filter = params["group"].get<std::string>();

        json entities = json::array();
        for (auto &[uu, entity] : doc.m_entities) {
            if (group_filter && entity->m_group != group_filter)
                continue;
            json ent_json = {
                    {"uuid", static_cast<std::string>(uu)},
                    {"type", entity->get_type_name()},
                    {"group", static_cast<std::string>(entity->m_group)},
                    {"construction", entity->m_construction},
            };
            if (!entity->m_name.empty())
                ent_json["name"] = entity->m_name;

            // Include geometry details per type
            if (auto *line2d = dynamic_cast<const EntityLine2D *>(entity.get())) {
                ent_json["p1"] = {line2d->m_p1.x, line2d->m_p1.y};
                ent_json["p2"] = {line2d->m_p2.x, line2d->m_p2.y};
                ent_json["wrkpl"] = static_cast<std::string>(line2d->m_wrkpl);
            }
            else if (auto *line3d = dynamic_cast<const EntityLine3D *>(entity.get())) {
                ent_json["p1"] = {line3d->m_p1.x, line3d->m_p1.y, line3d->m_p1.z};
                ent_json["p2"] = {line3d->m_p2.x, line3d->m_p2.y, line3d->m_p2.z};
            }
            else if (auto *circle2d = dynamic_cast<const EntityCircle2D *>(entity.get())) {
                ent_json["center"] = {circle2d->m_center.x, circle2d->m_center.y};
                ent_json["radius"] = circle2d->m_radius;
                ent_json["wrkpl"] = static_cast<std::string>(circle2d->m_wrkpl);
            }
            else if (auto *circle3d = dynamic_cast<const EntityCircle3D *>(entity.get())) {
                ent_json["center"] = {circle3d->m_center.x, circle3d->m_center.y, circle3d->m_center.z};
                ent_json["radius"] = circle3d->m_radius;
            }
            else if (auto *wrkpl = dynamic_cast<const EntityWorkplane *>(entity.get())) {
                ent_json["origin"] = {wrkpl->m_origin.x, wrkpl->m_origin.y, wrkpl->m_origin.z};
                ent_json["normal"] = {wrkpl->m_normal.w, wrkpl->m_normal.x, wrkpl->m_normal.y, wrkpl->m_normal.z};
            }

            entities.push_back(std::move(ent_json));
        }
        return entities;
    };

    // ===== Constraint queries =====

    m_methods["constraint.list"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.has_documents())
            throw std::runtime_error("No document open");
        auto &doc = core.get_current_document();

        UUID group_filter;
        if (params.contains("group"))
            group_filter = params["group"].get<std::string>();

        json constraints = json::array();
        for (auto &[uu, constraint] : doc.m_constraints) {
            if (group_filter && constraint->m_group != group_filter)
                continue;
            json c_json = {
                    {"uuid", static_cast<std::string>(uu)},
                    {"type", constraint->get_type_name()},
                    {"group", static_cast<std::string>(constraint->m_group)},
            };

            // Include datum info for constraints with values
            if (auto *datum = dynamic_cast<const IConstraintDatum *>(constraint.get())) {
                c_json["datum"] = datum->get_datum();
                c_json["datum_name"] = datum->get_datum_name();
                c_json["unit"] = datum->get_datum_unit() == DatumUnit::MM ? "mm" : "deg";
                c_json["measurement"] = datum->is_measurement();
            }

            constraints.push_back(std::move(c_json));
        }
        return constraints;
    };

    // ===== Export =====

    m_methods["export.stl"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.has_documents())
            throw std::runtime_error("No document open");
        if (!params.contains("path"))
            throw std::invalid_argument("missing 'path' parameter");

        auto &doc = core.get_current_document();
        auto path = params["path"].get<std::string>();

        // Find the last group with a solid model
        const SolidModel *model = nullptr;
        UUID target_group;
        if (params.contains("group")) {
            target_group = params["group"].get<std::string>();
        }

        for (auto *group : doc.get_groups_sorted()) {
            if (auto *solid_group = dynamic_cast<const IGroupSolidModel *>(group)) {
                if (solid_group->get_solid_model())
                    model = solid_group->get_solid_model();
            }
            if (target_group && group->m_uuid == target_group)
                break;
        }

        if (!model)
            throw std::runtime_error("No solid model found");

        model->export_stl(path);
        return {{"success", true}, {"path", path}};
    };

    m_methods["export.step"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.has_documents())
            throw std::runtime_error("No document open");
        if (!params.contains("path"))
            throw std::invalid_argument("missing 'path' parameter");

        auto &doc = core.get_current_document();
        auto &doc_info = core.get_current_idocument_info();
        auto path = params["path"].get<std::string>();

        STEPExporter exporter(doc_info.get_stem().c_str());

        auto groups_by_body = doc.get_groups_by_body();
        for (auto body_groups : groups_by_body) {
            const SolidModel *last_solid_model = nullptr;
            for (auto group : body_groups.groups) {
                if (auto gr = dynamic_cast<const IGroupSolidModel *>(group)) {
                    if (gr->get_solid_model())
                        last_solid_model = gr->get_solid_model();
                }
            }
            if (last_solid_model)
                last_solid_model->add_to_step_exporter(exporter, body_groups.body.m_name.c_str());
        }

        exporter.write(path);
        return {{"success", true}, {"path", path}};
    };

    // ===== Entity manipulation =====

    m_methods["entity.set_param"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.has_documents())
            throw std::runtime_error("No document open");
        if (!params.contains("uuid"))
            throw std::invalid_argument("missing 'uuid' parameter");
        if (!params.contains("point"))
            throw std::invalid_argument("missing 'point' parameter");
        if (!params.contains("axis"))
            throw std::invalid_argument("missing 'axis' parameter");
        if (!params.contains("value"))
            throw std::invalid_argument("missing 'value' parameter");

        UUID uu(params["uuid"].get<std::string>());
        auto &doc = core.get_current_document();
        auto &entity = doc.get_entity(uu);

        unsigned int point = json_to_uint(params["point"]);
        unsigned int axis = json_to_uint(params["axis"]);
        double value = json_to_double(params["value"]);

        if (!entity.is_valid_point(point))
            throw std::invalid_argument("Invalid point index");

        entity.set_param(point, axis, value);

        doc.set_group_solve_pending(entity.m_group);
        core.rebuild("ipc entity param changed");
        core.set_needs_save();
        m_editor.ipc_canvas_update();

        return {{"success", true}};
    };

    // ===== Solid model edge queries =====

    m_methods["solid_model.list_edges"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.has_documents())
            throw std::runtime_error("No document open");
        auto &doc = core.get_current_document();

        // Find the last solid model up to the specified group (or current group)
        UUID target_group;
        if (params.contains("group"))
            target_group = params["group"].get<std::string>();
        else
            target_group = core.get_current_group();

        const SolidModel *model = nullptr;
        for (auto *group : doc.get_groups_sorted()) {
            if (auto *solid_group = dynamic_cast<const IGroupSolidModel *>(group)) {
                if (solid_group->get_solid_model())
                    model = solid_group->get_solid_model();
            }
            if (group->m_uuid == target_group)
                break;
        }

        if (!model)
            throw std::runtime_error("No solid model found");

        json edges = json::array();
        for (auto &[idx, path] : model->m_edges) {
            json edge_json = {{"index", idx}};
            // Include start and end points of the edge
            if (path.size() >= 2) {
                edge_json["start"] = {path.front().x, path.front().y, path.front().z};
                edge_json["end"] = {path.back().x, path.back().y, path.back().z};
            }
            // Include midpoint for identification
            if (path.size() >= 3) {
                auto &mid = path[path.size() / 2];
                edge_json["mid"] = {mid.x, mid.y, mid.z};
            }
            edge_json["n_segments"] = path.size() - 1;
            edges.push_back(std::move(edge_json));
        }
        return edges;
    };

    m_methods["group.set_edges"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.has_documents())
            throw std::runtime_error("No document open");
        if (!params.contains("group"))
            throw std::invalid_argument("missing 'group' parameter");
        if (!params.contains("edges"))
            throw std::invalid_argument("missing 'edges' parameter");

        UUID group_uu(params["group"].get<std::string>());
        auto &doc = core.get_current_document();
        auto &group = doc.get_group(group_uu);

        auto *local_op = dynamic_cast<GroupLocalOperation *>(&group);
        if (!local_op)
            throw std::runtime_error("Group is not a fillet/chamfer (local operation) group");

        std::set<unsigned int> edges;
        for (auto &edge : params["edges"]) {
            edges.insert(json_to_uint(edge));
        }

        local_op->m_edges = edges;
        doc.set_group_update_solid_model_pending(group_uu);
        core.rebuild("ipc edges changed");
        core.set_needs_save();
        m_editor.ipc_canvas_update();

        return {{"success", true}, {"edge_count", edges.size()}};
    };

    m_methods["constraint.set_datum"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.has_documents())
            throw std::runtime_error("No document open");
        if (!params.contains("uuid"))
            throw std::invalid_argument("missing 'uuid' parameter");
        if (!params.contains("value"))
            throw std::invalid_argument("missing 'value' parameter");

        UUID uu(params["uuid"].get<std::string>());
        auto &doc = core.get_current_document();
        auto &constraint = doc.get_constraint(uu);

        auto *datum = dynamic_cast<IConstraintDatum *>(&constraint);
        if (!datum)
            throw std::runtime_error("Constraint does not have a settable value");

        double value = json_to_double(params["value"]);
        auto [range_min, range_max] = datum->get_datum_range();
        if (value < range_min || value > range_max)
            throw std::invalid_argument("Value out of range [" + std::to_string(range_min) + ", "
                                        + std::to_string(range_max) + "]");

        datum->set_datum(value);

        if (params.contains("measurement")) {
            datum->set_is_measurement(params["measurement"].get<bool>());
        }

        doc.set_group_solve_pending(constraint.m_group);
        core.rebuild("ipc constraint datum changed");
        core.set_needs_save();
        m_editor.ipc_canvas_update();

        return {
                {"success", true},
                {"value", datum->get_datum()},
                {"name", datum->get_datum_name()},
                {"unit", datum->get_datum_unit() == DatumUnit::MM ? "mm" : "deg"},
                {"measurement", datum->is_measurement()},
        };
    };

    // ===== Actions =====

    m_methods["action.list"] = [](const json &params, IpcSession &) -> json {
        json actions = json::array();
        for (auto &[atid, item] : action_catalog) {
            json entry;
            entry["name"] = item.name.full;
            if (auto action = std::get_if<ActionID>(&atid))
                entry["id"] = action_lut.lookup_reverse(*action);
            else if (auto tool = std::get_if<ToolID>(&atid))
                entry["id"] = tool_lut.lookup_reverse(*tool);
            entry["group"] = static_cast<int>(item.group);
            actions.push_back(entry);
        }
        return actions;
    };

    m_methods["action.trigger"] = [this](const json &params, IpcSession &) -> json {
        if (!params.contains("action"))
            throw std::invalid_argument("missing 'action' parameter");
        auto action_str = params["action"].get<std::string>();

        // try as ActionID first, then as ToolID
        auto action_opt = action_lut.lookup_opt(action_str);
        if (action_opt) {
            bool ok = m_editor.trigger_action(*action_opt, ActionSource::UNKNOWN);
            return {{"success", ok}};
        }

        auto tool_opt = tool_lut.lookup_opt(action_str);
        if (tool_opt) {
            bool ok = m_editor.trigger_action(*tool_opt, ActionSource::UNKNOWN);
            return {{"success", ok}};
        }

        throw std::invalid_argument("Unknown action or tool: " + action_str);
    };

    // ===== Document commands =====

    m_methods["document.new"] = [this](const json &params, IpcSession &) -> json {
        m_editor.trigger_action(ActionID::NEW_DOCUMENT, ActionSource::UNKNOWN);
        return {{"success", true}};
    };

    m_methods["document.save"] = [this](const json &params, IpcSession &) -> json {
        m_editor.trigger_action(ActionID::SAVE, ActionSource::UNKNOWN);
        return {{"success", true}};
    };

    m_methods["document.save_all"] = [this](const json &params, IpcSession &) -> json {
        m_editor.trigger_action(ActionID::SAVE_ALL, ActionSource::UNKNOWN);
        return {{"success", true}};
    };

    // ===== Undo/Redo =====

    m_methods["undo"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.can_undo())
            throw std::runtime_error("Nothing to undo");
        m_editor.trigger_action(ActionID::UNDO, ActionSource::UNKNOWN);
        return {{"success", true}};
    };

    m_methods["redo"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.can_redo())
            throw std::runtime_error("Nothing to redo");
        m_editor.trigger_action(ActionID::REDO, ActionSource::UNKNOWN);
        return {{"success", true}};
    };

    // ===== Tool driving (Phase 4) =====

    m_methods["tool.get_active"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.tool_is_active())
            return nullptr;
        return {
                {"tool_id", tool_lut.lookup_reverse(core.get_tool_id())},
        };
    };

    m_methods["tool.begin"] = [this](const json &params, IpcSession &) -> json {
        if (!params.contains("tool"))
            throw std::invalid_argument("missing 'tool' parameter");

        auto tool_str = params["tool"].get<std::string>();
        auto tool_opt = tool_lut.lookup_opt(tool_str);
        if (!tool_opt)
            throw std::invalid_argument("Unknown tool: " + tool_str);

        auto &core = m_editor.get_core();
        if (core.tool_is_active())
            throw std::runtime_error("A tool is already active");

        ToolArgs args;
        args.selection = parse_selection(params);

        ToolResponse resp = core.tool_begin(*tool_opt, args);
        m_editor.ipc_tool_process(resp);

        std::string result_str;
        switch (resp.result) {
        case ToolResponse::Result::NOP:
            result_str = "NOP";
            break;
        case ToolResponse::Result::END:
            result_str = "END";
            break;
        case ToolResponse::Result::COMMIT:
            result_str = "COMMIT";
            break;
        case ToolResponse::Result::REVERT:
            result_str = "REVERT";
            break;
        }
        return {{"result", result_str}, {"tool_active", core.tool_is_active()}};
    };

    m_methods["tool.set_cursor"] = [this](const json &params, IpcSession &) -> json {
        if (!params.contains("point") || !params["point"].is_array() || params["point"].size() != 3)
            throw std::invalid_argument("'point' must be an array of 3 numbers [x,y,z]");

        glm::dvec3 pos(params["point"][0].get<double>(), params["point"][1].get<double>(),
                       params["point"][2].get<double>());

        m_editor.set_ipc_cursor_override(pos);

        // Send a MOVE event so the active tool sees the new cursor position
        auto &core = m_editor.get_core();
        if (core.tool_is_active()) {
            ToolArgs args;
            args.type = ToolEventType::MOVE;
            ToolResponse resp = core.tool_update(args);
            m_editor.ipc_tool_process(resp);
        }

        return {{"success", true}};
    };

    m_methods["tool.click"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.tool_is_active())
            throw std::runtime_error("No tool is active");

        // Send a MOVE first so the tool sees the current cursor position
        {
            ToolArgs move_args;
            move_args.type = ToolEventType::MOVE;
            ToolResponse move_resp = core.tool_update(move_args);
            m_editor.ipc_tool_process(move_resp);
        }

        ToolArgs args;
        args.type = ToolEventType::ACTION;
        args.action = InToolActionID::LMB;

        ToolResponse resp = core.tool_update(args);
        m_editor.ipc_tool_process(resp);

        std::string result_str;
        switch (resp.result) {
        case ToolResponse::Result::NOP:
            result_str = "NOP";
            break;
        case ToolResponse::Result::END:
            result_str = "END";
            break;
        case ToolResponse::Result::COMMIT:
            result_str = "COMMIT";
            break;
        case ToolResponse::Result::REVERT:
            result_str = "REVERT";
            break;
        }
        return {{"result", result_str}, {"tool_active", core.tool_is_active()}};
    };

    m_methods["tool.action"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.tool_is_active())
            throw std::runtime_error("No tool is active");

        if (!params.contains("action"))
            throw std::invalid_argument("missing 'action' parameter");

        auto action_str = params["action"].get<std::string>();
        auto action = in_tool_action_lut.lookup(action_str, InToolActionID::LMB);

        ToolArgs args;
        args.type = ToolEventType::ACTION;
        args.action = action;

        ToolResponse resp = core.tool_update(args);
        m_editor.ipc_tool_process(resp);

        std::string result_str;
        switch (resp.result) {
        case ToolResponse::Result::NOP:
            result_str = "NOP";
            break;
        case ToolResponse::Result::END:
            result_str = "END";
            break;
        case ToolResponse::Result::COMMIT:
            result_str = "COMMIT";
            break;
        case ToolResponse::Result::REVERT:
            result_str = "REVERT";
            break;
        }
        return {{"result", result_str}, {"tool_active", core.tool_is_active()}};
    };

    m_methods["tool.cancel"] = [this](const json &params, IpcSession &) -> json {
        auto &core = m_editor.get_core();
        if (!core.tool_is_active())
            return {{"success", true}};

        ToolArgs args;
        args.type = ToolEventType::ACTION;
        args.action = InToolActionID::RMB;

        core.tool_update(args);
        return {{"success", true}, {"tool_active", core.tool_is_active()}};
    };

    // ===== Event subscriptions =====

    m_methods["events.subscribe"] = [](const json &params, IpcSession &session) -> json {
        if (!params.contains("events") || !params["events"].is_array())
            throw std::invalid_argument("missing 'events' array parameter");
        for (auto &ev : params["events"]) {
            session.m_subscriptions.insert(ev.get<std::string>());
        }
        return {{"subscribed", params["events"]}};
    };

    m_methods["events.unsubscribe"] = [](const json &params, IpcSession &session) -> json {
        if (params.contains("events") && params["events"].is_array()) {
            for (auto &ev : params["events"]) {
                session.m_subscriptions.erase(ev.get<std::string>());
            }
        }
        else {
            session.m_subscriptions.clear();
        }
        return {{"success", true}};
    };
}

} // namespace dune3d

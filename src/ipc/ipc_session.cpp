#include "ipc_session.hpp"
#include "ipc_server.hpp"
#include "logger/logger.hpp"
#include <glib-unix.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

namespace dune3d {

IpcSession::IpcSession(IpcServer &server, int fd) : m_server(server), m_fd(fd)
{
    m_source_id = g_unix_fd_add(
            m_fd, static_cast<GIOCondition>(G_IO_IN | G_IO_HUP | G_IO_ERR),
            [](int fd, GIOCondition cond, gpointer data) -> gboolean {
                return static_cast<IpcSession *>(data)->on_data(fd, cond) ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
            },
            this);
    Logger::log_info("IPC session connected", Logger::Domain::EDITOR);
}

IpcSession::~IpcSession()
{
    if (m_source_id)
        g_source_remove(m_source_id);
    if (m_fd >= 0)
        close(m_fd);
    Logger::log_info("IPC session disconnected", Logger::Domain::EDITOR);
}

bool IpcSession::on_data(int fd, unsigned int condition)
{
    if (condition & (G_IO_HUP | G_IO_ERR)) {
        m_source_id = 0;
        m_server.remove_session(this);
        return false;
    }

    char buf[4096];
    ssize_t n = read(m_fd, buf, sizeof(buf));
    if (n <= 0) {
        m_source_id = 0;
        m_server.remove_session(this);
        return false;
    }

    m_read_buffer.append(buf, n);

    size_t pos;
    while ((pos = m_read_buffer.find('\n')) != std::string::npos) {
        auto line = m_read_buffer.substr(0, pos);
        m_read_buffer.erase(0, pos + 1);
        if (!line.empty())
            process_line(line);
    }

    return true;
}

void IpcSession::process_line(const std::string &line)
{
    json response;
    json id = nullptr;

    try {
        auto request = json::parse(line);

        if (!request.contains("jsonrpc") || request["jsonrpc"] != "2.0" || !request.contains("method")) {
            response = {
                    {"jsonrpc", "2.0"}, {"error", {{"code", -32600}, {"message", "Invalid Request"}}}, {"id", nullptr}};
            send(response.dump() + "\n");
            return;
        }

        id = request.value("id", json(nullptr));
        auto method = request["method"].get<std::string>();
        auto params = request.value("params", json::object());

        auto result = m_server.dispatch(method, params, *this);
        response = {{"jsonrpc", "2.0"}, {"result", result}, {"id", id}};
    }
    catch (const json::parse_error &) {
        response = {{"jsonrpc", "2.0"}, {"error", {{"code", -32700}, {"message", "Parse error"}}}, {"id", nullptr}};
    }
    catch (const std::invalid_argument &e) {
        response = {{"jsonrpc", "2.0"},
                    {"error", {{"code", -32602}, {"message", std::string("Invalid params: ") + e.what()}}},
                    {"id", id}};
    }
    catch (const std::runtime_error &e) {
        response = {{"jsonrpc", "2.0"},
                    {"error", {{"code", -32603}, {"message", std::string("Internal error: ") + e.what()}}},
                    {"id", id}};
    }
    catch (const std::exception &e) {
        response = {{"jsonrpc", "2.0"},
                    {"error", {{"code", -32603}, {"message", std::string("Internal error: ") + e.what()}}},
                    {"id", id}};
    }

    send(response.dump() + "\n");
}

void IpcSession::send(const std::string &msg)
{
    if (m_fd < 0)
        return;

    ssize_t total = msg.size();
    ssize_t written = 0;
    while (written < total) {
        ssize_t n = write(m_fd, msg.data() + written, total - written);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            Logger::log_warning("IPC write failed: " + std::string(std::strerror(errno)), Logger::Domain::EDITOR);
            return;
        }
        written += n;
    }
}

} // namespace dune3d

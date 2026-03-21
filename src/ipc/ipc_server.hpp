#pragma once
#include <string>
#include <memory>
#include <list>
#include <map>
#include <functional>
#include <filesystem>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace dune3d {

class Editor;
class IpcSession;

class IpcServer {
public:
    explicit IpcServer(Editor &editor);
    ~IpcServer();

    IpcServer(const IpcServer &) = delete;
    IpcServer &operator=(const IpcServer &) = delete;

    json dispatch(const std::string &method, const json &params, IpcSession &session);
    void broadcast_notification(const std::string &method, const json &params = json::object());
    void remove_session(IpcSession *session);

    Editor &get_editor()
    {
        return m_editor;
    }

private:
    using Handler = std::function<json(const json &, IpcSession &)>;

    void setup_socket();
    void cleanup_socket();
    bool on_accept(int fd, unsigned int condition);
    void register_methods();
    void connect_signals();

    Editor &m_editor;
    int m_listen_fd = -1;
    unsigned int m_accept_source_id = 0;
    std::filesystem::path m_socket_path;

    std::list<std::unique_ptr<IpcSession>> m_sessions;
    std::map<std::string, Handler> m_methods;
};

} // namespace dune3d

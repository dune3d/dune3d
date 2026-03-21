#pragma once
#include <string>
#include <set>

namespace dune3d {

class IpcServer;

class IpcSession {
public:
    IpcSession(IpcServer &server, int fd);
    ~IpcSession();

    IpcSession(const IpcSession &) = delete;
    IpcSession &operator=(const IpcSession &) = delete;

    void send(const std::string &msg);

    std::set<std::string> m_subscriptions;

private:
    bool on_data(int fd, unsigned int condition);
    void process_line(const std::string &line);

    IpcServer &m_server;
    int m_fd;
    unsigned int m_source_id = 0;
    std::string m_read_buffer;
};

} // namespace dune3d

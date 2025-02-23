#pragma once
#include <deque>
#include <functional>
#include <string>
#include <tuple>
#include <cstdint>

namespace dune3d {
class Logger {
public:
    enum class Level { DEBUG, INFO, WARNING, CRITICAL };
    enum class Domain {
        UNSPECIFIED,
        DOCUMENT,
        EDITOR,
        TOOL,
        CORE,
        CANVAS,
        IMPORT,
        VERSION,
        PICTURE,
    };

    Logger();
    static Logger &get();
    static std::string level_to_string(Level level);
    static std::string domain_to_string(Domain domain);

    static void log_debug(const std::string &message, Domain domain = Domain::UNSPECIFIED,
                          const std::string &detail = "");
    static void log_info(const std::string &message, Domain domain = Domain::UNSPECIFIED,
                         const std::string &detail = "");
    static void log_warning(const std::string &message, Domain domain = Domain::UNSPECIFIED,
                            const std::string &detail = "");
    static void log_critical(const std::string &message, Domain domain = Domain::UNSPECIFIED,
                             const std::string &detail = "");

    class Item {
    public:
        Item(uint64_t s, Level l, const std::string &msg, Domain dom = Domain::UNSPECIFIED, const std::string &det = "")
            : seq(s), level(l), message(msg), domain(dom), detail(det)
        {
        }

        uint64_t seq;
        Level level;
        std::string message;
        Domain domain = Domain::UNSPECIFIED;
        std::string detail;
    };

    typedef std::function<void(const Item &it)> log_handler_t;

    void log(Level level, const std::string &message, Domain domain = Domain::UNSPECIFIED,
             const std::string &detail = "");
    void set_log_handler(log_handler_t handler);

private:
    log_handler_t handler = nullptr;
    std::deque<Item> buffer;
    uint64_t seq = 0;
};
} // namespace dune3d

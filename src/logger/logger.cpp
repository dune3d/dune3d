#include "logger.hpp"
#include <iostream>

namespace dune3d {
static Logger the_logger;

Logger::Logger()
{
}

Logger &Logger::get()
{
    return the_logger;
}

void Logger::log(Logger::Level l, const std::string &m, Logger::Domain d, const std::string &detail)
{
    std::cout << m << " " << detail << std::endl;
    if (handler) {
        handler(Item(seq++, l, m, d, detail));
    }
    else {
        buffer.emplace_back(seq++, l, m, d, detail + " (startup)");
    }
}

void Logger::log_debug(const std::string &message, Domain domain, const std::string &detail)
{
    get().log(Level::DEBUG, message, domain, detail);
}

void Logger::log_critical(const std::string &message, Domain domain, const std::string &detail)
{
    get().log(Level::CRITICAL, message, domain, detail);
}

void Logger::log_info(const std::string &message, Domain domain, const std::string &detail)
{
    get().log(Level::INFO, message, domain, detail);
}

void Logger::log_warning(const std::string &message, Domain domain, const std::string &detail)
{
    get().log(Level::WARNING, message, domain, detail);
}

void Logger::set_log_handler(Logger::log_handler_t h)
{
    if (handler)
        return;
    handler = h;
    for (const auto &it : buffer) {
        handler(it);
    }
}

std::string Logger::domain_to_string(Logger::Domain dom)
{
    switch (dom) {
    case Logger::Domain::BLOCK:
        return "Block";
    case Logger::Domain::BOARD:
        return "Board";
    case Logger::Domain::SCHEMATIC:
        return "Schematic";
    case Logger::Domain::TOOL:
        return "Tool";
    case Logger::Domain::CORE:
        return "Core";
    case Logger::Domain::CANVAS:
        return "Canvas";
    case Logger::Domain::IMP:
        return "Interactive manipulator";
    case Logger::Domain::IMPORT:
        return "Import";
    case Logger::Domain::VERSION:
        return "Version";
    case Logger::Domain::POOL_UPDATE:
        return "Pool update";
    case Logger::Domain::PICTURE:
        return "Picture";
    case Logger::Domain::PART:
        return "Part";
    case Logger::Domain::PROJECT:
        return "Project";
    case Logger::Domain::BLOCKS:
        return "Blocks";
    default:
        return "Unspecified";
    }
}

std::string Logger::level_to_string(Logger::Level lev)
{
    switch (lev) {
    case Logger::Level::CRITICAL:
        return "Critical";
    case Logger::Level::DEBUG:
        return "Debug";
    case Logger::Level::INFO:
        return "Info";
    case Logger::Level::WARNING:
        return "Warning";
    default:
        return "Unknown";
    }
}
} // namespace dune3d

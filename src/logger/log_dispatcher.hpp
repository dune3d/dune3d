#pragma once
#include <glibmm/dispatcher.h>
#include "logger.hpp"
#include <mutex>
#include <thread>

namespace dune3d {

class LogDispatcher {
public:
    LogDispatcher();
    void log(const Logger::Item &item);
    void set_handler(Logger::log_handler_t h);

private:
    Glib::Dispatcher dispatcher;
    Logger::log_handler_t handler;
    const std::thread::id main_thread_id;

    std::mutex mutex;
    std::list<Logger::Item> items;
};

} // namespace dune3d

#include "log_dispatcher.hpp"

namespace dune3d {
LogDispatcher::LogDispatcher() : main_thread_id(std::this_thread::get_id())
{
    dispatcher.connect([this] {
        if (!handler)
            return;
        std::list<Logger::Item> buf;
        {
            std::lock_guard<std::mutex> guard(mutex);
            buf.splice(buf.begin(), items);
        }
        for (const auto &it : buf) {
            handler(it);
        }
    });
}

void LogDispatcher::set_handler(Logger::log_handler_t h)
{
    handler = h;
}

void LogDispatcher::log(const Logger::Item &item)
{
    if (std::this_thread::get_id() == main_thread_id) {
        if (handler)
            handler(item);
    }
    else {
        {
            std::lock_guard<std::mutex> guard(mutex);
            items.push_front(item);
        }
        dispatcher.emit();
    }
}

} // namespace dune3d

#pragma once
#include "common/object_descr.hpp"
#include "logger.hpp"
#include "util/uuid.hpp"
#include <map>
#include <string>
#include <tuple>

namespace dune3d {
template <typename T, typename... Args1>
void load_and_log(std::map<UUID, T> &map, ObjectType type, std::tuple<Args1...> args,
                  Logger::Domain dom = Logger::Domain::UNSPECIFIED)
{
    load_and_log(map, object_descriptions.at(type).name, std::forward<std::tuple<Args1...>>(args), dom);
}

template <typename T, typename... Args1>
void load_and_log(std::map<UUID, T> &map, const std::string &type, std::tuple<Args1...> args,
                  Logger::Domain dom = Logger::Domain::UNSPECIFIED)
{
    auto uu = std::get<0>(args);
    try {
        map.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward<std::tuple<Args1...>>(args));
    }
    catch (const std::exception &e) {
        Logger::log_warning("couldn't load " + type + " " + (std::string)uu, dom, e.what());
    }
    catch (...) {
        Logger::log_warning("couldn't load " + type + " " + (std::string)uu, dom);
    }
}
} // namespace dune3d

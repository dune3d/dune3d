#pragma once
#include "logger.hpp"
#include <map>
#include <string>
#include <tuple>

namespace dune3d {

#define CATCH_LOG(level, msg, domain)                                                                                  \
    catch (const std::exception &e)                                                                                    \
    {                                                                                                                  \
        Logger::get().log(level, msg, domain, e.what());                                                               \
    }                                                                                                                  \
    catch (...)                                                                                                        \
    {                                                                                                                  \
        Logger::get().log(level, msg, domain, "unknown exception");                                                    \
    }

} // namespace dune3d

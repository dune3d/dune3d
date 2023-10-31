#pragma once

namespace dune3d {
// see https://awesomekling.github.io/Serenity-C++-patterns-The-Badge/
template <typename T> class Badge {
    friend T;
    Badge() = default;
};

} // namespace dune3d

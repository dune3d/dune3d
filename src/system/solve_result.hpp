#pragma once

namespace dune3d {
enum class SolveResult {
    OKAY,
    DIDNT_CONVERGE,
    REDUNDANT_OKAY,
    REDUNDANT_DIDNT_CONVERGE,
    TOO_MANY_UNKNOWNS,
};
}

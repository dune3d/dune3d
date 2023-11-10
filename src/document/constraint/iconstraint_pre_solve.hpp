#pragma once

namespace dune3d {
class Document;
class IConstraintPreSolve {
public:
    virtual void pre_solve(Document &doc) const = 0;
};
} // namespace dune3d

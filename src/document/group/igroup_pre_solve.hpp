#pragma once

namespace dune3d {
class Document;
class IGroupPreSolve {
public:
    virtual void pre_solve(Document &doc) const = 0;
};
} // namespace dune3d

#pragma once

namespace dune3d {
class Document;
class SolidModel;
class SolidModelOcc;
class IGroupSolidModel {
public:
    virtual const SolidModel *get_solid_model() const = 0;
    virtual void update_solid_model(const Document &doc) = 0;
    enum class Operation { UNION, DIFFERENCE, INTERSECTION };
    virtual Operation get_operation() const = 0;
};
} // namespace dune3d

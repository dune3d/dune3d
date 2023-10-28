#pragma once
#include <string>

namespace dune3d {
class Document;
class IConstraintDatum {
public:
    virtual double get_datum() const = 0;
    virtual void set_datum(double d) = 0;
    virtual std::string get_datum_name() const = 0;
};
} // namespace dune3d

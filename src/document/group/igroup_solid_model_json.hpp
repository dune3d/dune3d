#pragma once

namespace dune3d {

NLOHMANN_JSON_SERIALIZE_ENUM(IGroupSolidModel::Operation,
                             {
                                     {IGroupSolidModel::Operation::DIFFERENCE, "difference"},
                                     {IGroupSolidModel::Operation::UNION, "union"},
                                     {IGroupSolidModel::Operation::INTERSECTION, "intersection"},
                             })
}

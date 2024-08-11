#include "constraint_parallel.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraintt_impl.hpp"

namespace dune3d {
ConstraintParallel::ConstraintParallel(const UUID &uu) : Base(uu)
{
}

ConstraintParallel::ConstraintParallel(const UUID &uu, const json &j)
    : Base(uu, j), m_entity1(j.at("entity1").get<UUID>()), m_entity2(j.at("entity2").get<UUID>()),
      m_wrkpl(j.at("wrkpl").get<UUID>()), m_val(j.at("val").get<double>())
{
}

json ConstraintParallel::serialize() const
{
    json j = Constraint::serialize();
    j["entity1"] = m_entity1;
    j["entity2"] = m_entity2;
    j["wrkpl"] = m_wrkpl;
    j["val"] = m_val;
    return j;
}

} // namespace dune3d

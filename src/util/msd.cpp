#include "msd.hpp"
#include <cmath>

namespace dune3d {
MSD::MSD()
{
    params.mass = 0.002;
    params.damping = .2;
    params.springyness = .15;
}

void MSD::reset(double init)
{
    t = 0;
    a = 0;
    v = 0;
    s = init;
    target = init;
}

bool MSD::step(double ts)
{
    auto f_friction = -v * params.damping;
    auto f_spring = (target - s) / params.springyness;
    a = (f_spring + f_friction) / params.mass;
    v += a * ts;
    s += v * ts;
    t += ts;
    if ((std::abs(target - s) < 1e-3) && (std::abs(v) < 1e-2)) {
        s = target;
        return false;
    }
    return true;
}

bool MSD::run_to(double time, double ts)
{
    while (t < time) {
        if (!step(ts))
            return false;
    }
    return true;
}


double MSD::get_s() const
{
    return s;
}
double MSD::get_t() const
{
    return t;
}

} // namespace dune3d

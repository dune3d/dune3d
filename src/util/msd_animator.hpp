#pragma once
#include "msd.hpp"

namespace dune3d {
class MSDAnimator {
public:
    bool step(double time);
    double get_s() const;
    double get_s_delta();
    void start(double init = 0);
    float target = 0;
    bool is_running() const;

    void set_params(const MSD::Params &p);
    const MSD::Params &get_params() const;

private:
    MSD msd;
    bool running = false;
    double start_time = 0;
    double m_last_s = 0;
};
} // namespace dune3d

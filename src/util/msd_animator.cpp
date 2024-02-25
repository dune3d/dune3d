#include "msd_animator.hpp"

namespace dune3d {
bool MSDAnimator::step(double frame_time)
{
    if (running == false)
        return false;

    if (start_time == 0) {
        start_time = frame_time;
    }
    double t = (frame_time - start_time);
    msd.target = target;
    if (!msd.run_to(t + (1. / 60.), 5e-3)) {
        running = false;
        return false;
    }
    return true;
}

double MSDAnimator::get_s() const
{
    return msd.get_s();
}

double MSDAnimator::get_s_delta()
{
    const auto s = get_s();
    const auto delta = s - m_last_s;
    m_last_s = s;
    return delta;
}

bool MSDAnimator::is_running() const
{
    return running;
}

void MSDAnimator::start(double init)
{
    if (running)
        return;
    msd.reset(init);
    running = true;
    start_time = 0;
    target = init;
    m_last_s = init;
}

void MSDAnimator::set_params(const MSD::Params &p)
{
    msd.params = p;
}

const MSD::Params &MSDAnimator::get_params() const
{
    return msd.params;
}

} // namespace dune3d

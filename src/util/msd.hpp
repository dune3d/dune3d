#pragma once

namespace dune3d {
class MSD { // mass spring damper system simulation
public:
    MSD();
    bool run_to(double time, double ts);
    bool step(double ts);
    void reset(double init = 0);
    double get_s() const;
    double get_t() const;

    struct Params {
        double mass = 0.003;      // kg
        double damping = .21;     // kg/s
        double springyness = .25; // newton/m
    };

    Params params;

    double target = 0; // m
private:
    double a = 0; // m/s²
    double v = 0; // m/s
    double s = 0; // m
    double t = 0; // s
};
} // namespace dune3d

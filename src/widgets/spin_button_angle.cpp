#include "spin_button_angle.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"
#include <iomanip>

namespace dune3d {

SpinButtonAngle::SpinButtonAngle() : Gtk::SpinButton()
{
    set_wrap(false);
    set_width_chars(6);
    double step = 1;
    double page = 15;
    set_increments(step, page);

    // The value normally is only within 0..65536, but the range has to be larger to simulate the wrapping by
    // temporarily allowing smaller/larger values and then readjusting them.
    set_range(-page, 360 + page);
    signal_input().connect(sigc::mem_fun(*this, &SpinButtonAngle::on_input), true);
    signal_output().connect(sigc::mem_fun(*this, &SpinButtonAngle::on_output), true);
    signal_wrapped().connect(sigc::mem_fun(*this, &SpinButtonAngle::on_wrapped), true);
}

bool SpinButtonAngle::on_output()
{
    auto adj = get_adjustment();
    double v = adj->get_value();

    // When changing the value by scroll wheel, on_output() is called for each step, but on_input() isn't until later.
    // Without this check, negative angles could be displayed or the range limits could be hit while scrolling.
    if (v < 0) {
        v += 360.0;
        set_value(v);
    }
    else if (v >= 360.0) {
        v -= 360.0;
        set_value(v);
    }

    std::stringstream stream;
    stream.imbue(get_locale());
    stream << std::fixed << std::setprecision(2) << v << "°";

    set_text(stream.str());
    return true;
}

int SpinButtonAngle::on_input(double &v)
{
    auto txt = get_text().raw();
    std::replace(txt.begin(), txt.end(), ',', '.');
    int64_t va = 0;
    try {
        double d = std::fmod(std::stod(txt), 360.0);
        if (d < 0) {
            d += 360.0;
        }
        va = d;
        v = va;
    }
    catch (const std::exception &e) {
        return false;
    }
    return true;
}

void SpinButtonAngle::on_wrapped()
{
    auto adj = get_adjustment();
    if (adj->get_value() == adj->get_upper()) {
        // Wrap 0 -> maximum
        adj->set_value(std::floor(360 / adj->get_step_increment()) * adj->get_step_increment());
    }
}
} // namespace dune3d

#include "spin_button_ratio.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"
#include <iomanip>

namespace dune3d {

SpinButtonRatio::SpinButtonRatio() : Gtk::SpinButton()
{
    set_width_chars(10);
    set_increments(0.1, 0.1);
    set_range(ConstraintLengthRatio::s_min_ratio, ConstraintLengthRatio::s_max_ratio);
    signal_input().connect(sigc::mem_fun(*this, &SpinButtonRatio::on_input), true);
    signal_output().connect(sigc::mem_fun(*this, &SpinButtonRatio::on_output), true);
}

int SpinButtonRatio::on_input(double &new_value)
{
    auto txt = get_text().raw();
    std::replace(txt.begin(), txt.end(), ',', '.');
    try {
        new_value = std::stod(txt);
        return true;
    }
    catch (const std::exception &) {
        return INPUT_ERROR;
    }
}

bool SpinButtonRatio::on_output()
{
    auto adj = get_adjustment();
    double value = adj->get_value();

    int prec = 3;
    int64_t ivalue = std::llround(std::abs(value) * 1e6);
    if (ivalue % 1000)
        prec = 5;

    std::stringstream stream;
    stream.imbue(get_locale());
    stream << std::fixed << std::setprecision(prec) << value << " ×";

    set_text(stream.str());
    return true;
}

} // namespace dune3d

#include "spin_button_dim.hpp"
#include "util/util.hpp"
// #include "util/gtk_util.hpp"
#include <iomanip>

namespace dune3d {
SpinButtonDim::SpinButtonDim() : Gtk::SpinButton()
{
    set_increments(.1, .01);
    set_width_chars(11);
    signal_input().connect(sigc::mem_fun(*this, &SpinButtonDim::on_input), true);
    signal_output().connect(sigc::mem_fun(*this, &SpinButtonDim::on_output), true);
}

bool is_close(double a, double b)
{
    auto r = a / b;
    return std::abs(1 - r) < 1e-3;
}

bool SpinButtonDim::on_output()
{
    auto adj = get_adjustment();
    double value = adj->get_value();

    int prec = 3;
    {
        int64_t ivalue = abs(round(value * 1e6));
        if (ivalue % 1000)
            prec = 5;
    }

    double min, max;
    get_range(min, max);
    std::stringstream stream;
    stream.imbue(get_locale());
    if (value < 0)
        stream << "−"; // this is U+2212 MINUS SIGN, has same width as +
    else if (value >= 0 && min < 0)
        stream << "+";

    stream << std::fixed << std::setprecision(prec) << std::abs(value) << " mm";

    set_text(stream.str());
    return true;
}

static bool is_dec(char c)
{
    return (c == '.') || (c == ',');
}

enum class Operation { INVALID, ADD, SUB, MUL, DIV, AVG };

static Operation op_from_char(char c)
{
    switch (c) {
    case '-':
        return Operation::SUB;
    case '+':
        return Operation::ADD;
    case '*':
        return Operation::MUL;
    case '/':
        return Operation::DIV;
    case '|':
        return Operation::AVG;

    default:
        return Operation::INVALID;
    }
}

static bool op_is_mul(Operation op)
{
    switch (op) {
    case Operation::MUL:
    case Operation::DIV:
        return true;
    default:
        return false;
    }
}

static bool is_minus(gunichar c)
{
    if (c == '-' || c == 0x2212) // U+2212 MINUS SIGN
        return true;
    else
        return false;
}


static int64_t parse_str(const Glib::ustring &s)
{
    int64_t value1 = 0;
    int64_t value = 0;
    int64_t mul = 1000000;
    int sign = 1;
    enum class State { SIGN, INT, DECIMAL, UNIT, UNIT_M };
    auto state = State::SIGN;
    bool parsed_any = false;
    Operation operation = Operation::INVALID;
    for (const auto c : s) {
        switch (state) {
        case State::SIGN:
            if (is_minus(c)) {
                sign = -1;
                state = State::INT;
            }
            else if (is_dec(c)) {
                state = State::DECIMAL;
                mul /= 10;
            }
            else if (isdigit(c)) {
                value += mul * (c - '0');
                state = State::INT;
                parsed_any = true;
            }
            break;


        case State::INT:
            if (isdigit(c)) {
                value *= 10;
                value += mul * (c - '0');
                parsed_any = true;
            }
            else if (is_dec(c)) {
                state = State::DECIMAL;
                mul /= 10;
            }
            else if (op_from_char(c) != Operation::INVALID && operation == Operation::INVALID) {
                operation = op_from_char(c);
                value1 = value * sign;
                sign = 1;
                mul = 1000000;
                value = 0;
                state = State::SIGN;
            }
            else if (c == 'i' && !op_is_mul(operation)) {
                value *= 25.4;
                state = State::UNIT;
            }
            else if (c == 'm' && !op_is_mul(operation)) {
                state = State::UNIT_M;
            }
            break;

        case State::DECIMAL:
            if (isdigit(c)) {
                value += mul * (c - '0');
                mul /= 10;
                parsed_any = true;
            }
            else if (op_from_char(c) != Operation::INVALID && operation == Operation::INVALID) {
                operation = op_from_char(c);
                value1 = value * sign;
                sign = 1;
                mul = 1000000;
                value = 0;
                state = State::SIGN;
            }
            else if (c == 'i' && !op_is_mul(operation)) {
                value *= 25.4;
                state = State::UNIT;
            }
            else if (c == 'm' && !op_is_mul(operation)) {
                state = State::UNIT_M;
            }
            break;

        case State::UNIT_M:
            if (c == 'i') {
                value *= 25.4 / 1000.0;
            }
            state = State::UNIT;
            /* fall through */

        case State::UNIT:
            if (op_from_char(c) != Operation::INVALID && operation == Operation::INVALID) {
                operation = op_from_char(c);
                value1 = value * sign;
                sign = 1;
                mul = 1000000;
                value = 0;
                state = State::SIGN;
            }

            break;
        }
    }
    if (!parsed_any)
        throw std::runtime_error("parse error");
    value *= sign;
    switch (operation) {
    case Operation::ADD:
        return value1 + value;
    case Operation::SUB:
        return value1 - value;
    case Operation::AVG:
        return (value1 + value) / 2;
    case Operation::DIV:
        return value1 / (((double)value) / 1e6);
    case Operation::MUL:
        return value1 * (((double)value) / 1e6);
    default:
        return value;
    }
}

int SpinButtonDim::on_input(double &v)
{
    auto txt = get_text();
    int64_t va = 0;
    try {
        va = parse_str(txt);
        v = va / 1e6;
    }
    catch (const std::exception &e) {
        return false;
    }
    return true;
}
} // namespace dune3d

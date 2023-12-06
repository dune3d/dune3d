#include "dune3d_application.hpp"
#include "util/exception_util.hpp"

int main(int argc, char *argv[])
{
    auto application = dune3d::Dune3DApplication::create();
    dune3d::install_signal_exception_handler();

    // Start the application, showing the initial window,
    // and opening extra views for any files that it is asked to open,
    // for instance as a command-line parameter.
    // run() will return when the last window has been closed.
    return application->run(argc, argv);
}

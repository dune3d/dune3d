#include "dune3d_application.hpp"

int main(int argc, char *argv[])
{
    auto application = dune3d::Dune3DApplication::create();

    // Start the application, showing the initial window,
    // and opening extra views for any files that it is asked to open,
    // for instance as a command-line parameter.
    // run() will return when the last window has been closed.
    return application->run(argc, argv);
}

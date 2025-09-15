#include "dune3d_application.hpp"
#include "util/exception_util.hpp"

#ifdef HAVE_WAYLAND_PROXY
#include "wayland-proxy.h"
#endif

int main(int argc, char *argv[])
{

#ifdef HAVE_WAYLAND_PROXY
    // WaylandProxy::SetVerbose(false);

    // Create and run Wayland proxy in extra thread
    std::unique_ptr<WaylandProxy> wayland_proxy;
    if (getenv("WAYLAND_DISPLAY")) {
        wayland_proxy = WaylandProxy::Create();
        if (wayland_proxy) {
            wayland_proxy->RunThread();
        }
    }
#endif

    auto application = dune3d::Dune3DApplication::create();
    dune3d::install_signal_exception_handler();

    // Start the application, showing the initial window,
    // and opening extra views for any files that it is asked to open,
    // for instance as a command-line parameter.
    // run() will return when the last window has been closed.
    auto rv = application->run(argc, argv);
#ifdef HAVE_WAYLAND_PROXY
    wayland_proxy = nullptr;
#endif
    return rv;
}

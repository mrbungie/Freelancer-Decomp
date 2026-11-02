// Default render backend: registers nothing. The client reports a render-pipeline startup failure.
// Exists so the tree links before a real backend (sokol, ...) is written.
#include "fl/platform.h"
#include "fl/render_backend.h"

namespace fl { namespace render
{
    namespace { void* G_Window = nullptr; }
    namespace { IWindowSurface* G_Surface = nullptr; }
    void SetWindowSurface(IWindowSurface* s) { G_Surface = s; }
    IWindowSurface* GetWindowSurface() { return G_Surface; }
    void SetNativeWindow(void* w) { G_Window = w; }
    void* GetNativeWindow() { return G_Window; }

    WindowKind RequiredWindowKind() { return WindowKind::None; }
    bool RegisterBackend(ICOManager*)
    {
        ReportUnimplemented("render backend (configure with -DFL_RENDER_BACKEND=sokol once implemented)");
        return false;
    }
}}

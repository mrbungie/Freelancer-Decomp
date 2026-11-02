// Render backend extension point.
//
// The whole engine reaches the GPU through the DACOM component named "DirectX8"
// (kRenderPipelineComponentName, render/IRenderPipeline.h): retail's RP8.dll registered a factory under that
// name at DLL_PROCESS_ATTACH, and DALib::Pipeline is obtained by CreateInstance on it.
//
// A backend is a static library in src/platform/render/<name>/ that implements ONE function:
//
//     bool fl::render::RegisterBackend(ICOManager* dacom);
//
// which must register a factory for the pipeline under kRenderPipelineComponentName with
// priority kRp8RegisterPriority (what RP8's DllMain does, RP8.cpp ~6588). The front end calls it once,
// before WinMain. Choose the backend with -DFL_RENDER_BACKEND=<name> (null | sokol | ...).
//
// To add one (e.g. sokol on Metal/GL/D3D11, or whatever an Xbox 360 port provides):
//   1. copy src/platform/render/sokol/ to src/platform/render/<name>/
//   2. implement the interfaces listed in its README against your GPU API
//   3. configure with -DFL_RENDER_BACKEND=<name>
#pragma once

class ICOManager;

namespace fl { namespace render
{
    // What the backend draws into. The front end creates the window accordingly.
    enum class WindowKind { None, OpenGL, Metal };
    WindowKind RequiredWindowKind();

    // The drawable surface the front end gives the backend. For Metal: the CAMetalLayer of the
    // window (SDL_Metal_GetLayer); for OpenGL: a current GL context. Installed before the pipeline starts.
    class IWindowSurface
    {
    public:
        virtual ~IWindowSurface() {}
        virtual bool CreateContext(void* window) = 0;      // OpenGL: create + make current. Metal: create the view
        virtual void* NativeLayer(void* window) = 0;       // Metal: CAMetalLayer*; otherwise null
        virtual void Present(void* window) = 0;            // OpenGL swap; Metal presents through the command buffer
        virtual void DrawableSize(void* window, int* width, int* height) = 0;
    };
    void SetWindowSurface(IWindowSurface* surface);
    IWindowSurface* GetWindowSurface();

    // Returns false if the backend could not register (the client then fails at
    // StartupRenderPipeline with its normal fatal-error dialog).
    bool RegisterBackend(ICOManager* dacom);

    // Native window handle the backend should draw into (SDL_Window* in the SDL front end).
    void SetNativeWindow(void* window);
    void* GetNativeWindow();
}}

// Thin Objective-C++ bridge so the (plain C++) device code never touches Metal types directly.
// Implemented in sokol_impl_metal.mm; only built on Apple platforms.
#pragma once
namespace flmetal
{
    struct Context;
    // `caMetalLayer` is the window's CAMetalLayer* (SDL_Metal_GetLayer). Returns null on failure.
    Context* Create(void* caMetalLayer);
    const void* Device(Context*);                  // id<MTLDevice>, for sg_environment.metal.device
    // Acquires the next drawable and a matching depth/stencil texture sized w x h.
    // Returns false when no drawable is available (skip the frame).
    bool BeginFrame(Context*, int w, int h, const void** drawable, const void** depthStencil);
    void Destroy(Context*);
    void RequestCapture(const char* path);                     // next presented frame is written to a PNG
    void CaptureIfRequested(Context*, const void* sokolQueue); // call right after sg_commit()
}

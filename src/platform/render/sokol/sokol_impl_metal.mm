// sokol_gfx implementation (Metal backend) + the CAMetalLayer bridge. Objective-C++, ARC.
#define SOKOL_IMPL
#define SOKOL_METAL
#include "sokol_gfx.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include "metal_bridge.h"

namespace flmetal
{
    struct Context
    {
        CAMetalLayer* layer = nil;
        id<MTLDevice> device = nil;
        id<MTLTexture> depth = nil;
        id<CAMetalDrawable> drawable = nil;   // kept alive until the next frame (sokol presents it on commit)
        int width = 0, height = 0;
    };

    Context* Create(void* caMetalLayer)
    {
        if (!caMetalLayer) return nullptr;
        Context* c = new Context();
        c->layer = (__bridge CAMetalLayer*)caMetalLayer;
        c->device = MTLCreateSystemDefaultDevice();
        if (!c->device) { delete c; return nullptr; }
        c->layer.device = c->device;
        c->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        c->layer.framebufferOnly = YES;
        return c;
    }

    const void* Device(Context* c) { return (__bridge const void*)c->device; }

    bool BeginFrame(Context* c, int w, int h, const void** drawable, const void** depthStencil)
    {
        if (w <= 0 || h <= 0) return false;
        if (w != c->width || h != c->height || !c->depth)
        {
            c->layer.drawableSize = CGSizeMake(w, h);
            MTLTextureDescriptor* d =
                [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8
                                                                   width:(NSUInteger)w
                                                                  height:(NSUInteger)h
                                                               mipmapped:NO];
            d.usage = MTLTextureUsageRenderTarget;
            d.storageMode = MTLStorageModePrivate;
            c->depth = [c->device newTextureWithDescriptor:d];
            c->width = w;
            c->height = h;
        }
        c->drawable = [c->layer nextDrawable];
        if (!c->drawable) return false;
        *drawable = (__bridge const void*)c->drawable;
        *depthStencil = (__bridge const void*)c->depth;
        return true;
    }

    void Destroy(Context* c) { delete c; }
}

// sokol_gfx implementation (Metal backend) + the CAMetalLayer bridge. Objective-C++, ARC.
#define SOKOL_IMPL
#define SOKOL_METAL
#include "sokol_gfx.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include "metal_bridge.h"
#import <ImageIO/ImageIO.h>
#import <CoreServices/CoreServices.h>
#include <string>
#include <mutex>

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
        c->layer.framebufferOnly = NO;   // readable, so FL_CONTROL 'shot' can grab the frame
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

    // ---- framebuffer capture (debug): the game writes its own frame to a PNG, no window grab needed ----
    static std::mutex g_capMutex;
    static std::string g_capPath;
    void RequestCapture(const char* path) { std::lock_guard<std::mutex> l(g_capMutex); g_capPath = path ? path : ""; }

    // Call right after sg_commit(): sokol has queued the frame and presented it; a blit queued behind it on
    // the same command queue sees the finished image.
    void CaptureIfRequested(Context* c, const void* sokolQueue)
    {
        std::string path;
        { std::lock_guard<std::mutex> l(g_capMutex); path.swap(g_capPath); }
        if (path.empty() || !c->drawable || !sokolQueue) return;
        id<MTLTexture> src = c->drawable.texture;
        NSUInteger w = src.width, h = src.height;
        MTLTextureDescriptor* d = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                     width:w height:h mipmapped:NO];
        d.storageMode = MTLStorageModeShared;
        id<MTLTexture> dst = [c->device newTextureWithDescriptor:d];
        id<MTLCommandQueue> q = (__bridge id<MTLCommandQueue>)sokolQueue;
        id<MTLCommandBuffer> cb = [q commandBuffer];
        id<MTLBlitCommandEncoder> blit = [cb blitCommandEncoder];
        [blit copyFromTexture:src toTexture:dst];
        [blit endEncoding];
        [cb commit];
        [cb waitUntilCompleted];
        std::string px(w * h * 4, 0);
        [dst getBytes:px.data() bytesPerRow:w * 4 fromRegion:MTLRegionMake2D(0, 0, w, h) mipmapLevel:0];
        CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
        CGContextRef g = CGBitmapContextCreate(px.data(), w, h, 8, w * 4, cs,
                                               kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little);
        CGImageRef img = CGBitmapContextCreateImage(g);
        NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
        CGImageDestinationRef dest = CGImageDestinationCreateWithURL((__bridge CFURLRef)url, kUTTypePNG, 1, nullptr);
        CGImageDestinationAddImage(dest, img, nullptr);
        CGImageDestinationFinalize(dest);
        CFRelease(dest); CGImageRelease(img); CGContextRelease(g); CGColorSpaceRelease(cs);
        fprintf(stderr, "[fl] captured %s\n", path.c_str());
    }
}

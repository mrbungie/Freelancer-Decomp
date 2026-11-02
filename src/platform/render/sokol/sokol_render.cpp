// sokol_gfx render backend: the DACOM component "GfxPipeline" (kRenderPipelineComponentName) (see fl/render_backend.h, README.md).
//
// STATE OF IMPLEMENTATION
//   real   : component/aggregation plumbing, device bring-up on Metal (sokol_gfx + CAMetalLayer), clear + present,
//            all fixed-function state storage (matrices, viewport, render/texture-stage states, lights,
//            material), CPU-side textures (create/lock/unlock/level data), vertex/index buffers (CPU memory).
//   TODO   : actual drawing (DrawPrimitive*/DrawIndexedPrimitive*): translate FVF + render states + texture
//            stages into sokol pipelines/shaders; upload textures/buffers to the GPU; gamma; render targets.
//            Every unimplemented method comes from sokol_device_stubs.inc and logs once ("render stub: ...").
#include "fl/platform.h"
#include "fl/handles.h"
#include "fl/text.h"
#if defined(__APPLE__)
fl::text::IRasterizer* FlCreateCoreTextRasterizer();
#endif
#include "fl/render_backend.h"

namespace fl { namespace render
{
    namespace { void* G_Window = nullptr; IWindowSurface* G_Surface = nullptr; }
    void SetNativeWindow(void* w) { G_Window = w; }
    void* GetNativeWindow() { return G_Window; }
    void SetWindowSurface(IWindowSurface* s) { G_Surface = s; }
    IWindowSurface* GetWindowSurface() { return G_Surface; }
#if defined(__APPLE__)
    WindowKind RequiredWindowKind() { return WindowKind::Metal; }
#else
    WindowKind RequiredWindowKind() { return WindowKind::OpenGL; }   // TODO: sokol_impl_gl.cpp for non-Apple hosts
#endif
}}

#if !defined(FL_HAVE_SOKOL)
namespace fl { namespace render
{
    bool RegisterBackend(ICOManager*)
    {
        ReportUnimplemented("sokol render backend (vendor sokol into third_party/sokol to enable)");
        return false;
    }
}}
#else

#include "sokol_gfx.h"      // declarations only; the implementation lives in sokol_impl_metal.mm
#if defined(__APPLE__)
#  include "metal_bridge.h"
#endif

#include "dacom.h"
#include "dacom_iids.h"
#include "render/IRenderPipeline.h"
#include "d3dx8tex.h"       // _D3DXIMAGE_INFO

#include "sokol_draw_support.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace
{
    // The pipeline's texture formats are retail "pfenum" ids (RP8 kPfToD3d / client pixel_format.cpp), not D3DFMT_*:
    //   1 P8, 2 R8G8B8, 3 R5G6B5, 4 X1R5G5B5, 5 A4R4G4B4, 6 A1R5G5B5, 7 A8R8G8B8, 8 X8R8G8B8,
    //   10..14 DXT1..DXT5, 15.. bump/luminance formats (unsupported here).
    unsigned BytesPerPixel(unsigned pf)
    {
        static const unsigned kBytes[9] = {0, 1, 3, 2, 2, 2, 2, 4, 4};
        return pf < 9 ? kBytes[pf] : 0;
    }
    bool IsDxt(unsigned pf) { return pf >= 10 && pf <= 14; }
    size_t LevelBytes(unsigned pf, unsigned w, unsigned h)
    {
        if (IsDxt(pf)) return (size_t)((w + 3) / 4) * ((h + 3) / 4) * (pf == 10 ? 8 : 16);
        unsigned bpp = BytesPerPixel(pf);
        return (size_t)w * h * (bpp ? bpp : 4);
    }

    struct FTexture
    {
        unsigned width, height, format, levels;
        std::vector<std::vector<unsigned char>> data;
        sg_image img = {};
        sg_view view = {};
        bool dirty = true;
    };
    struct FBuffer { unsigned fvf = 0; unsigned count = 0; std::vector<unsigned char> bytes; };
    struct FIndexBuf { unsigned count = 0; std::vector<unsigned short> idx; };

    std::set<std::string>& StubSeen() { static std::set<std::string> s; return s; }

    class FSokolDeviceBase :
        public IRenderPipeline, public IVertexBufferManager, public IRPDraw, public IRPIndexBuffer,
        public IRPVertexBuffer, public IGammaControl, public IRPTexture, public IAggregateComponent
    {
    protected:
        static GENRESULT Stub(const char* what)
        {
            if (StubSeen().insert(what).second) fprintf(stderr, "[fl] render stub: %s\n", what);
            return GR_OK;
        }
    public:
#include "sokol_device_stubs.inc"
        GENRESULT DACOM_API Initialize() override { return GR_OK; }
    };

    class FSokolDevice;

    // Non-delegating IUnknown (aggregation inner), as RP8's FRp8Inner.
    class FInner : public IDAComponent
    {
    public:
        FSokolDevice* Owner = nullptr;
        uint32_t Refs = 1;
        GENRESULT DACOM_API QueryInterface(const char* iid, void** out) override;
        uint32_t DACOM_API AddRef() override { return ++Refs; }
        uint32_t DACOM_API Release() override;
    };

    class FSokolDevice final : public FSokolDeviceBase
    {
        // ---------------------------------------------------------------- state
        FInner inner_;
        IDAComponent* outer_ = nullptr;
        bool started_ = false, sgReady_ = false, inScene_ = false, passOpen_ = false, clearPending_ = false;
        HWND hwnd_ = nullptr;
        int mode_[10] = {};
        int pipeState_[16] = {};
        uint32_t ability_[0x15] = {};
        int vpX_ = 0, vpY_ = 0, vpW_ = 0, vpH_ = 0;
        float depthMin_ = 0, depthMax_ = 1;
        int winX_ = 0, winY_ = 0, winW_ = 0, winH_ = 0;
        float world_[12] = {}, view_[12] = {}, modelview_[12] = {}, proj_[16] = {};
        float texXform_[8][16] = {};
        int renderState_[256] = {};
        int stageState_[8][64] = {};
        void* stageTex_[8] = {};
        int gamma_ = 0;
        std::map<void*, D3DLIGHT8> lights_;
        std::map<void*, int> lightEnabled_;
        D3DMATERIAL8 material_ = {};
        std::set<FTexture*> textures_;
        std::set<FBuffer*> vbs_;
        std::set<FIndexBuf*> ibs_;
        std::vector<unsigned char> vertexMem_;
#if defined(__APPLE__)
        flmetal::Context* metal_ = nullptr;
        const void* drawable_ = nullptr;
        const void* depth_ = nullptr;
#endif
        sg_color clearColor_ = {0.0f, 0.0f, 0.0f, 1.0f};

        // ---- drawing state
        sg_shader shader_ = {};
        sg_image whiteImg_ = {};
        sg_view whiteView_ = {};
        std::map<uint64_t, sg_pipeline> pipelines_;
        std::map<uint32_t, sg_sampler> samplers_;
        std::vector<sg_buffer> garbage_;
        float worldV_[16], viewV_[16], projV_[16];
        float viewRaw_[12] = {1,0,0, 0,1,0, 0,0,1, 0,0,0}, worldRaw_[12] = {1,0,0, 0,1,0, 0,0,1, 0,0,0}, modelviewRaw_[12] = {1,0,0, 0,1,0, 0,0,1, 0,0,0};
        bool viewMarker_ = false;
        FBuffer* selVb_ = nullptr;
        FIndexBuf* selIb_ = nullptr;
        unsigned selIbBase_ = 0;
        std::map<unsigned, FBuffer*> poolVb_;
        FIndexBuf* scratchIb_ = nullptr;
        unsigned scratchUsed_ = 0;
        int drawW_ = 0, drawH_ = 0;
        unsigned drawCalls_ = 0;

        void BeginPassIfNeeded()
        {
            if (!sgReady_ || passOpen_) return;
            int w = winW_ ? winW_ : 640, h = winH_ ? winH_ : 480;
            if (fl::render::IWindowSurface* s = fl::render::GetWindowSurface()) s->DrawableSize(hwnd_, &w, &h);
#if defined(__APPLE__)
            if (!flmetal::BeginFrame(metal_, w, h, &drawable_, &depth_)) return;   // no drawable: skip this frame
#endif
            drawW_ = w; drawH_ = h;
            sg_pass pass = {};
            pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;   // a fresh drawable has undefined contents; the game clears every frame anyway
            pass.action.colors[0].clear_value = clearColor_;
            pass.action.depth.load_action = SG_LOADACTION_CLEAR;
            pass.action.depth.clear_value = 1.0f;
            pass.swapchain.width = w;
            pass.swapchain.height = h;
            pass.swapchain.sample_count = 1;
            pass.swapchain.color_format = SG_PIXELFORMAT_BGRA8;
            pass.swapchain.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
#if defined(__APPLE__)
            pass.swapchain.metal.current_drawable = drawable_;
            pass.swapchain.metal.depth_stencil_texture = depth_;
#endif
            sg_begin_pass(&pass);
            passOpen_ = true;
            clearPending_ = false;
        }
        void EndPassIfOpen() { if (passOpen_) { sg_end_pass(); passOpen_ = false; } }

    public:
        FSokolDevice()
        {
            inner_.Owner = this;
            ability_[3] = 256ul << 20;        // video memory bytes (callers shift >> 20)
            ability_[0x11] = 0;               // no max display width
            // Capabilities RP8 derives from D3DCAPS8, here for a modern GPU (see RP8.cpp ~1410..1440).
            ability_[1] = 1; ability_[2] = 1;          // raster caps the gamma/blit paths test
            ability_[4] = 1;                            // hardware transform & lighting
            ability_[5] = 0;                            // square-only textures: no
            ability_[6] = 256; ability_[7] = 256;       // max texture width / height (RP8 clamps to 0x100 with LimitTextureSize; BigImage tiles exactly at this size)
            ability_[8] = 0;                            // environment mapping: not rendered yet
            ability_[16] = 1;                           // trilinear filtering
            fldraw::Identity(worldV_); fldraw::Identity(viewV_); fldraw::Identity(projV_);
            ResetRenderStatesToDefaults();
        }
        FInner* Inner() { return &inner_; }
        void SetOuter(IDAComponent* o) { outer_ = o ? o : &inner_; }

        // ------------------------------------------------------------ identity
        GENRESULT QueryLocal(const char* iid, void** out)
        {
            if (!out) return GR_GENERIC;
            *out = nullptr;
            IDAComponent* face = nullptr;
            auto is = [&](const char* n) { return strcmp(iid, n) == 0; };
            if (is(Iids::RenderPipeline8B)) face = static_cast<IRenderPipeline*>(this);
            else if (is(Iids::VertexBufferManager)) face = static_cast<IVertexBufferManager*>(this);
            else if (is(Iids::RPDraw)) face = static_cast<IRPDraw*>(this);
            else if (is(Iids::RPIndexBuffer)) face = static_cast<IRPIndexBuffer*>(this);
            else if (is(Iids::RPVertexBuffer)) face = static_cast<IRPVertexBuffer*>(this);
            else if (is(Iids::GammaControl)) face = static_cast<IGammaControl*>(this);
            else if (is(Iids::RPTexture)) face = static_cast<IRPTexture*>(this);
            else if (is(Iids::AggregateComponent)) face = static_cast<IAggregateComponent*>(this);
            if (!face) return GR_INTERFACE_UNSUPPORTED;
            face->AddRef();
            *out = face;
            return GR_OK;
        }
        GENRESULT DACOM_API QueryInterface(const char* iid, void** out) override
        {
            if (outer_ && outer_ != &inner_) return outer_->QueryInterface(iid, out);
            return QueryLocal(iid, out);
        }
        uint32_t DACOM_API AddRef() override
        {
            if (outer_ && outer_ != &inner_) return outer_->AddRef();
            return ++inner_.Refs;
        }
        uint32_t DACOM_API Release() override { return outer_->Release(); }


        // ================================================================ drawing core
        FIndexBuf* ScratchIb()
        {
            if (!scratchIb_) { scratchIb_ = new FIndexBuf(); ibs_.insert(scratchIb_); }
            return scratchIb_;
        }

        void InitGpu()
        {
            sg_shader_desc sd = {};
            sd.vertex_func.source = fldraw::kShaderMsl;   sd.vertex_func.entry = "vs_main";
            sd.fragment_func.source = fldraw::kShaderMsl; sd.fragment_func.entry = "fs_main";
            sd.uniform_blocks[0].stage = SG_SHADERSTAGE_VERTEX;   sd.uniform_blocks[0].size = sizeof(fldraw::VsUniforms); sd.uniform_blocks[0].msl_buffer_n = 0;
            sd.uniform_blocks[1].stage = SG_SHADERSTAGE_FRAGMENT; sd.uniform_blocks[1].size = sizeof(fldraw::FsUniforms); sd.uniform_blocks[1].msl_buffer_n = 0;
            for (int i = 0; i < 2; ++i)
            {
                sd.views[i].texture.stage = SG_SHADERSTAGE_FRAGMENT;
                sd.views[i].texture.image_type = SG_IMAGETYPE_2D;
                sd.views[i].texture.sample_type = SG_IMAGESAMPLETYPE_FLOAT;
                sd.views[i].texture.msl_texture_n = (uint8_t)i;
                sd.samplers[i].stage = SG_SHADERSTAGE_FRAGMENT;
                sd.samplers[i].sampler_type = SG_SAMPLERTYPE_FILTERING;
                sd.samplers[i].msl_sampler_n = (uint8_t)i;
                sd.texture_sampler_pairs[i].stage = SG_SHADERSTAGE_FRAGMENT;
                sd.texture_sampler_pairs[i].view_slot = (uint8_t)i;
                sd.texture_sampler_pairs[i].sampler_slot = (uint8_t)i;
            }
            sd.label = "fixed-function";
            shader_ = sg_make_shader(&sd);
            const uint32_t white = 0xFFFFFFFFu;
            sg_image_desc id = {};
            id.width = 1; id.height = 1; id.pixel_format = SG_PIXELFORMAT_BGRA8;
            id.data.ptr = &white; id.data.size = 4;
            whiteImg_ = sg_make_image(&id);
            sg_view_desc vd = {}; vd.texture.image = whiteImg_;
            whiteView_ = sg_make_view(&vd);
            if (sg_query_shader_state(shader_) != SG_RESOURCESTATE_VALID) fprintf(stderr, "[fl] render: shader creation FAILED\n");
        }

        void FreeGpuTexture(FTexture* t)
        {
            if (t->view.id) { sg_destroy_view(t->view); t->view = {}; }
            if (t->img.id) { sg_destroy_image(t->img); t->img = {}; }
        }

        static sg_pixel_format GpuFormat(unsigned pf)
        {
            switch (pf)
            {
            case 10: return SG_PIXELFORMAT_BC1_RGBA;
            case 11: case 12: return SG_PIXELFORMAT_BC2_RGBA;
            case 13: case 14: return SG_PIXELFORMAT_BC3_RGBA;
            default: return SG_PIXELFORMAT_BGRA8;
            }
        }

        sg_view EnsureGpuTexture(FTexture* t)
        {
            if (!t->dirty && t->view.id) return t->view;
            FreeGpuTexture(t);
            t->dirty = false;
            const sg_pixel_format gf = GpuFormat(t->format);
            if (t->format > 14 || (t->format == 9) || t->width == 0 || t->height == 0) return whiteView_;
            if (IsDxt(t->format) && !sg_query_pixelformat(gf).sample) return whiteView_;
            unsigned maxLevels = 1; for (unsigned m = t->width > t->height ? t->width : t->height; m > 1; m >>= 1) ++maxLevels;
            unsigned levels = t->levels < maxLevels ? t->levels : maxLevels;
            if (getenv("FL_TEX_NOMIPS")) levels = 1;
            std::vector<unsigned char> blob;
            unsigned lw = t->width, lh = t->height;
            for (unsigned l = 0; l < levels; ++l)
            {
                if (IsDxt(t->format))
                {
                    const auto& src = t->data[l];
                    blob.insert(blob.end(), src.begin(), src.end());
                }
                else
                {
                    const size_t off = blob.size();
                    blob.resize(off + (size_t)lw * lh * 4);
                    fldraw::ToBgra8(t->format, lw, lh, t->data[l].data(), blob.data() + off);
                }
                lw = lw > 1 ? lw / 2 : 1; lh = lh > 1 ? lh / 2 : 1;
            }
            if (getenv("FL_DUMP_TEX") && !IsDxt(t->format))
            {   // debug: 32-bit BMP of level 0 (BGRA, top-down)
                static int idx = 0; char path[128]; snprintf(path, sizeof(path), "/tmp/texdump/%02d_%ux%u_f%u.bmp", idx++, t->width, t->height, t->format);
                if (FILE* f = fopen(path, "wb"))
                {
                    const uint32_t sz = 54 + t->width * t->height * 4; uint8_t h[54] = {'B','M'};
                    memcpy(h + 2, &sz, 4); const uint32_t off = 54, hs = 40, w = t->width, hh = (uint32_t)(-(int)t->height); const uint16_t pl = 1, bp = 32;
                    memcpy(h + 10, &off, 4); memcpy(h + 14, &hs, 4); memcpy(h + 18, &w, 4); memcpy(h + 22, &hh, 4); memcpy(h + 26, &pl, 2); memcpy(h + 28, &bp, 2);
                    fwrite(h, 1, 54, f); fwrite(blob.data(), 1, (size_t)t->width * t->height * 4, f); fclose(f);
                }
            }
            sg_image_desc id = {};
            id.type = SG_IMAGETYPE_2D; id.width = (int)t->width; id.height = (int)t->height; id.num_mipmaps = (int)levels;
            id.pixel_format = gf; id.data.ptr = blob.data(); id.data.size = blob.size();
            t->img = sg_make_image(&id);
            sg_view_desc vd = {}; vd.texture.image = t->img;
            t->view = sg_make_view(&vd);
            return t->view.id ? t->view : whiteView_;
        }

        sg_view StageView(int st)
        {
            FTexture* t = (FTexture*)stageTex_[st];
            if (getenv("FL_TRACE_TEX"))
            {
                static std::set<void*> seen;
                if (seen.insert(stageTex_[st]).second)
                {
                    size_t nz = 0; if (t && textures_.count(t)) for (unsigned char c : t->data[0]) nz += c != 0;
                    fprintf(stderr, "[fl] tex stage%d %p valid=%d %ux%u fmt=%u lv=%u nonzero=%zu\n", st, stageTex_[st], (int)(t && textures_.count(t)), t && textures_.count(t) ? t->width : 0, t && textures_.count(t) ? t->height : 0, t && textures_.count(t) ? t->format : 0, t && textures_.count(t) ? t->levels : 0, nz);
                }
            }
            if (!t || !textures_.count(t)) return whiteView_;
            return EnsureGpuTexture(t);
        }

        sg_sampler StageSampler(int st)
        {
            const int* s = stageState_[st];
            auto filt = [](int v) { return v == 1 || v == 0 ? SG_FILTER_NEAREST : SG_FILTER_LINEAR; };
            auto wrap = [](int v) { switch (v) { case 2: return SG_WRAP_MIRRORED_REPEAT; case 3: return SG_WRAP_CLAMP_TO_EDGE; case 4: return SG_WRAP_CLAMP_TO_BORDER; default: return SG_WRAP_REPEAT; } };
            const uint32_t key = (uint32_t)s[13] | ((uint32_t)s[14] << 4) | ((uint32_t)s[16] << 8) | ((uint32_t)s[17] << 12) | ((uint32_t)s[18] << 16);
            auto it = samplers_.find(key);
            if (it != samplers_.end()) return it->second;
            sg_sampler_desc d = {};
            d.mag_filter = filt(s[16]); d.min_filter = filt(s[17]);
            d.mipmap_filter = (s[18] >= 2) ? SG_FILTER_LINEAR : SG_FILTER_NEAREST;
            if (s[18] <= 0) d.max_lod = 0.25f;
            d.wrap_u = wrap(s[13]); d.wrap_v = wrap(s[14]);
            sg_sampler smp = sg_make_sampler(&d);
            samplers_[key] = smp;
            return smp;
        }

        static sg_blend_factor BlendFactor(int v, sg_blend_factor def)
        {
            switch (v)
            {
            case 1: return SG_BLENDFACTOR_ZERO;
            case 2: return SG_BLENDFACTOR_ONE;
            case 3: return SG_BLENDFACTOR_SRC_COLOR;
            case 4: return SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
            case 5: return SG_BLENDFACTOR_SRC_ALPHA;
            case 6: return SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            case 7: return SG_BLENDFACTOR_DST_ALPHA;
            case 8: return SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
            case 9: return SG_BLENDFACTOR_DST_COLOR;
            case 10: return SG_BLENDFACTOR_ONE_MINUS_DST_COLOR;
            case 11: return SG_BLENDFACTOR_SRC_ALPHA_SATURATED;
            }
            return def;
        }
        static sg_compare_func CompareFunc(int v)
        {
            switch (v)
            {
            case 1: return SG_COMPAREFUNC_NEVER;   case 2: return SG_COMPAREFUNC_LESS;
            case 3: return SG_COMPAREFUNC_EQUAL;   case 4: return SG_COMPAREFUNC_LESS_EQUAL;
            case 5: return SG_COMPAREFUNC_GREATER; case 6: return SG_COMPAREFUNC_NOT_EQUAL;
            case 7: return SG_COMPAREFUNC_GREATER_EQUAL;
            }
            return SG_COMPAREFUNC_ALWAYS;
        }

        sg_pipeline GetPipeline(unsigned fvf, const fldraw::FvfLayout& L, sg_primitive_type prim, bool indexed)
        {
            const bool blend = renderState_[27] != 0;
            const bool zon = renderState_[7] != 0;
            const int cull = renderState_[22];
            const int cw = renderState_[168] & 0xF;
            const int src = renderState_[19], dst = renderState_[20], zf = renderState_[23];
            const bool zw = renderState_[14] != 0;
            uint64_t key = fvf;
            key = key * 8 + (uint64_t)prim;
            key = key * 2 + (indexed ? 1 : 0);
            key = key * 2 + (blend ? 1 : 0);
            key = key * 16 + (uint64_t)(src & 15);
            key = key * 16 + (uint64_t)(dst & 15);
            key = key * 2 + (zon ? 1 : 0);
            key = key * 16 + (uint64_t)(zf & 15);
            key = key * 2 + (zw ? 1 : 0);
            key = key * 4 + (uint64_t)(cull & 3);
            key = key * 16 + (uint64_t)cw;
            auto it = pipelines_.find(key);
            if (it != pipelines_.end()) return it->second;

            sg_pipeline_desc pd = {};
            pd.shader = shader_;
            pd.layout.buffers[0].stride = (int)L.stride;
            pd.layout.attrs[0].format = L.rhw ? SG_VERTEXFORMAT_FLOAT4 : SG_VERTEXFORMAT_FLOAT3;
            // absent attributes read from offset 0 (their data is ignored via the uniforms / format flags)
            pd.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3; pd.layout.attrs[1].offset = L.normal >= 0 ? L.normal : 0;
            pd.layout.attrs[2].format = SG_VERTEXFORMAT_UBYTE4N; pd.layout.attrs[2].offset = L.diffuse >= 0 ? L.diffuse : 0;
            pd.layout.attrs[3].format = SG_VERTEXFORMAT_UBYTE4N; pd.layout.attrs[3].offset = L.specular >= 0 ? L.specular : 0;
            pd.layout.attrs[4].format = SG_VERTEXFORMAT_FLOAT2; pd.layout.attrs[4].offset = L.uv[0] >= 0 ? L.uv[0] : 0;
            pd.layout.attrs[5].format = SG_VERTEXFORMAT_FLOAT2; pd.layout.attrs[5].offset = L.uv[1] >= 0 ? L.uv[1] : (L.uv[0] >= 0 ? L.uv[0] : 0);
            pd.primitive_type = prim;
            pd.index_type = indexed ? SG_INDEXTYPE_UINT16 : SG_INDEXTYPE_NONE;
            pd.depth.compare = zon ? CompareFunc(zf) : SG_COMPAREFUNC_ALWAYS;
            pd.depth.write_enabled = zon && zw;
            pd.colors[0].pixel_format = SG_PIXELFORMAT_BGRA8;
            pd.color_count = 1;
            pd.colors[0].write_mask = cw ? (sg_color_mask)cw : SG_COLORMASK_NONE;
            if (blend)
            {
                pd.colors[0].blend.enabled = true;
                pd.colors[0].blend.src_factor_rgb = pd.colors[0].blend.src_factor_alpha = BlendFactor(src, SG_BLENDFACTOR_ONE);
                pd.colors[0].blend.dst_factor_rgb = pd.colors[0].blend.dst_factor_alpha = BlendFactor(dst, SG_BLENDFACTOR_ZERO);
            }
            pd.face_winding = SG_FACEWINDING_CW;
            pd.cull_mode = cull == 3 ? SG_CULLMODE_BACK : (cull == 2 ? SG_CULLMODE_FRONT : SG_CULLMODE_NONE);
            pd.label = "fixed-function";
            sg_pipeline pip = sg_make_pipeline(&pd);
            if (sg_query_pipeline_state(pip) != SG_RESOURCESTATE_VALID) fprintf(stderr, "[fl] render: pipeline creation failed (fvf 0x%x)\n", fvf);
            pipelines_[key] = pip;
            return pip;
        }

        // Single funnel for every draw call. `idx` (optional) are 16-bit indices relative to `verts`.
        GENRESULT Submit(uint32_t prim, unsigned fvf, const unsigned char* verts, unsigned nverts, const uint16_t* idx, unsigned nidx)
        {
            if (!sgReady_ || !verts || nverts == 0) return GR_GENERIC;
            const fldraw::FvfLayout L = fldraw::MakeLayout(fvf);
            if (!L.pos || L.stride == 0) return GR_GENERIC;
            if (TraceCam()) fprintf(stderr, "[cam] draw fvf=0x%x n=%u tex0=%p world.t=(%g %g %g) view.t=(%g %g %g) viewRaw=[%g %g %g | %g %g %g | %g %g %g]\n", fvf, nverts, stageTex_[0], worldV_[12], worldV_[13], worldV_[14], viewV_[12], viewV_[13], viewV_[14], viewRaw_[0], viewRaw_[1], viewRaw_[2], viewRaw_[3], viewRaw_[4], viewRaw_[5], viewRaw_[9], viewRaw_[10], viewRaw_[11]);
            if (TraceCam() && !L.rhw && (fvf == 0x102 || fvf == 0x142 || fvf == 0x112))
            {   // NDC bounding box of the mesh under the current world/view/projection
                float wv[16], m[16]; fldraw::Mul(wv, worldV_, viewV_); fldraw::Mul(m, wv, projV_);
                float mnx = 1e30f, mny = 1e30f, mxx = -1e30f, mxy = -1e30f; unsigned front = 0;
                float mnpx = 1e30f, mxpx = -1e30f, mnpy = 1e30f, mxpy = -1e30f, mnpz = 1e30f, mxpz = -1e30f;
                for (unsigned i = 0; i < nverts; ++i)
                {
                    const float* v = (const float*)(verts + (size_t)i * L.stride);
                    mnpx = fminf(mnpx, v[0]); mxpx = fmaxf(mxpx, v[0]); mnpy = fminf(mnpy, v[1]); mxpy = fmaxf(mxpy, v[1]); mnpz = fminf(mnpz, v[2]); mxpz = fmaxf(mxpz, v[2]);
                    const float cx = v[0] * m[0] + v[1] * m[4] + v[2] * m[8] + m[12], cy = v[0] * m[1] + v[1] * m[5] + v[2] * m[9] + m[13];
                    const float cw = v[0] * m[3] + v[1] * m[7] + v[2] * m[11] + m[15];
                    if (cw <= 0) continue;
                    ++front; mnx = fminf(mnx, cx / cw); mxx = fmaxf(mxx, cx / cw); mny = fminf(mny, cy / cw); mxy = fmaxf(mxy, cy / cw);
                }
                if (L.diffuse >= 0) { unsigned long long sa = 0, sr = 0; for (unsigned i = 0; i < nverts; ++i) { const uint32_t c = *(const uint32_t*)(verts + (size_t)i * L.stride + L.diffuse); sa += c >> 24; sr += (c >> 16) & 255; } fprintf(stderr, "[cam]   mean vertex alpha=%.1f red=%.1f\n", (double)sa / nverts, (double)sr / nverts); }
                fprintf(stderr, "[cam]   model bbox x[%g %g] y[%g %g] z[%g %g]; in front: %u/%u; ndc x[%g %g] y[%g %g]\n", mnpx, mxpx, mnpy, mxpy, mnpz, mxpz, front, nverts, mnx, mxx, mny, mxy);
            }
            BeginPassIfNeeded();
            if (!passOpen_) return GR_OK;

            std::vector<uint16_t> tmp;
            sg_primitive_type sp;
            switch (prim)
            {
            case 1: sp = SG_PRIMITIVETYPE_POINTS; break;
            case 2: sp = SG_PRIMITIVETYPE_LINES; break;
            case 3: sp = SG_PRIMITIVETYPE_LINE_STRIP; break;
            case 5: sp = SG_PRIMITIVETYPE_TRIANGLE_STRIP; break;
            default: sp = SG_PRIMITIVETYPE_TRIANGLES; break;
            }
            unsigned count = idx ? nidx : nverts;
            if (prim == 6)   // triangle fan -> list
            {
                const unsigned n = idx ? nidx : nverts;
                for (unsigned i = 2; i < n; ++i)
                {
                    tmp.push_back(idx ? idx[0] : 0); tmp.push_back(idx ? idx[i - 1] : (uint16_t)(i - 1)); tmp.push_back(idx ? idx[i] : (uint16_t)i);
                }
                idx = tmp.data(); count = (unsigned)tmp.size(); sp = SG_PRIMITIVETYPE_TRIANGLES;
            }
            if (count == 0) return GR_GENERIC;

            sg_buffer_desc bd = {};
            bd.data.ptr = verts; bd.data.size = (size_t)nverts * L.stride;
            sg_buffer vb = sg_make_buffer(&bd);
            sg_buffer ib = {};
            const bool indexed = idx != nullptr;
            if (indexed)
            {
                sg_buffer_desc id = {};
                id.usage.vertex_buffer = false; id.usage.index_buffer = true;
                id.data.ptr = idx; id.data.size = (size_t)count * 2;
                ib = sg_make_buffer(&id);
            }

            sg_apply_pipeline(GetPipeline(fvf, L, sp, indexed));

            // viewport (logical -> drawable pixels)
            const float lw = winW_ > 0 ? (float)winW_ : (vpW_ > 0 ? (float)(vpX_ + vpW_) : (float)drawW_);
            const float lh = winH_ > 0 ? (float)winH_ : (vpH_ > 0 ? (float)(vpY_ + vpH_) : (float)drawH_);
            const float sx = lw > 0 ? drawW_ / lw : 1.0f, sy = lh > 0 ? drawH_ / lh : 1.0f;
            if (vpW_ > 0 && vpH_ > 0) sg_apply_viewportf(vpX_ * sx, vpY_ * sy, vpW_ * sx, vpH_ * sy, true);
            else sg_apply_viewportf(0, 0, (float)drawW_, (float)drawH_, true);

            sg_bindings b = {};
            b.vertex_buffers[0] = vb;
            if (indexed) b.index_buffer = ib;
            for (int st = 0; st < 2; ++st) { b.views[st] = StageView(st); b.samplers[st] = StageSampler(st); }
            sg_apply_bindings(&b);

            fldraw::VsUniforms vu = {};
            float wv[16];
            fldraw::Mul(wv, worldV_, viewV_);
            fldraw::Mul(vu.mvp, wv, projV_);
            vu.misc[0] = vpW_ > 0 ? (float)vpW_ : lw;
            vu.misc[1] = vpH_ > 0 ? (float)vpH_ : lh;
            vu.misc[2] = L.rhw ? 1.0f : 0.0f;
            memcpy(vu.world, worldV_, sizeof(vu.world));
            {   // fixed-function lighting (D3D8 model): enabled lights, material, global ambient
                const bool lit = renderState_[137] != 0 && L.normal >= 0 && !L.rhw;
                vu.flags[0] = lit ? 1.0f : 0.0f;
                vu.flags[1] = (renderState_[141] != 0 && renderState_[145] == 1) ? 1.0f : 0.0f;
                vu.flags[2] = (renderState_[141] != 0 && renderState_[147] == 1) ? 1.0f : 0.0f;
                vu.flags[3] = L.diffuse >= 0 ? 1.0f : 0.0f;
                const uint32_t ga = (uint32_t)renderState_[139];
                vu.gamb[0] = ((ga >> 16) & 255) / 255.0f; vu.gamb[1] = ((ga >> 8) & 255) / 255.0f; vu.gamb[2] = (ga & 255) / 255.0f;
                auto col = [](float* d, const D3DCOLORVALUE& c) { d[0] = c.r; d[1] = c.g; d[2] = c.b; d[3] = c.a; };
                col(vu.mat[0], material_.Diffuse); col(vu.mat[1], material_.Ambient); col(vu.mat[2], material_.Emissive);
                int n = 0;
                if (lit)
                    for (auto& kv : lights_)
                    {
                        auto en = lightEnabled_.find(kv.first);
                        if (en == lightEnabled_.end() || !en->second || n >= 8) continue;
                        const D3DLIGHT8& l = kv.second;
                        float (*o)[4] = vu.lights[n++];
                        col(o[0], l.Diffuse); col(o[1], l.Ambient);
                        o[2][0] = l.Position.x; o[2][1] = l.Position.y; o[2][2] = l.Position.z; o[2][3] = l.Range;
                        // With viewspace lights (pipeline state 7) the world matrix carries the engine's Z flip, so lights get the same flip.
                        const int flipZ = 1;   // engine lights are authored for the engine's -Z-forward space; D3D's frame is Z-flipped
                        o[3][0] = l.Direction.x; o[3][1] = l.Direction.y; o[3][2] = flipZ ? -l.Direction.z : l.Direction.z; o[3][3] = (float)l.Type;
                        o[4][0] = l.Attenuation0; o[4][1] = l.Attenuation1; o[4][2] = l.Attenuation2; o[4][3] = l.Falloff;
                        o[5][0] = cosf(l.Theta * 0.5f); o[5][1] = cosf(l.Phi * 0.5f);
                    }
                vu.misc[3] = (float)n;
                static int lightLogs = 0;
                if (getenv("FL_TRACE_LIGHTS") && lit && lightLogs++ < 6)
                {
                    fprintf(stderr, "[fl] lights: registered=%zu enabled=%d ambient=%08x\n", lights_.size(), n, ga);
                    for (int i = 0; i < n; ++i)
                        fprintf(stderr, "    light%d type=%g dir=(%g %g %g) pos=(%g %g %g) diff=(%g %g %g) range=%g\n", i, vu.lights[i][3][3], vu.lights[i][3][0], vu.lights[i][3][1], vu.lights[i][3][2], vu.lights[i][2][0], vu.lights[i][2][1], vu.lights[i][2][2], vu.lights[i][0][0], vu.lights[i][0][1], vu.lights[i][0][2], vu.lights[i][2][3]);
                }
            }
            sg_range vr = {&vu, sizeof(vu)};
            sg_apply_uniforms(0, &vr);

            fldraw::FsUniforms fu = {};
            const uint32_t tf = (uint32_t)renderState_[60];
            fu.tfactor[0] = ((tf >> 16) & 255) / 255.0f; fu.tfactor[1] = ((tf >> 8) & 255) / 255.0f;
            fu.tfactor[2] = (tf & 255) / 255.0f; fu.tfactor[3] = ((tf >> 24) & 255) / 255.0f;
            fu.ctl[0] = renderState_[15] ? 1.0f : 0.0f; fu.ctl[1] = (float)renderState_[25];
            fu.ctl[2] = (renderState_[24] & 255) / 255.0f; fu.ctl[3] = (L.diffuse >= 0 || vu.flags[0] > 0.5f) ? 1.0f : 0.0f;
            for (int st = 0; st < 2; ++st)
            {
                const int* ss = stageState_[st];
                fu.st[st][0][0] = (float)ss[1]; fu.st[st][0][1] = (float)ss[2]; fu.st[st][0][2] = (float)ss[3]; fu.st[st][0][3] = (float)ss[4];
                fu.st[st][1][0] = (float)ss[5]; fu.st[st][1][1] = (float)ss[6]; fu.st[st][1][2] = (ss[11] & 0xFFFF) == 1 ? 1.0f : 0.0f;
            }
            sg_range fr = {&fu, sizeof(fu)};
            sg_apply_uniforms(1, &fr);

            sg_draw(0, (int)count, 1);
            if (getenv("FL_TRACE_PASSES"))
            {
                static std::set<std::pair<void*, void*>> seen;
                if (seen.insert({stageTex_[0], stageTex_[1]}).second)
                {
                    auto fmtOf = [&](void* h) { FTexture* t = (FTexture*)h; return (t && textures_.count(t)) ? (int)t->format : -1; };
                    if (fvf == 0x102) fprintf(stderr, "[fl] pass-mats: world.t=(%g %g %g) world.diag=(%g %g %g) view.t=(%g %g %g) proj=[%g %g %g %g | %g %g %g %g] vp=%d,%d,%d,%d z=%g..%g\n", worldV_[12], worldV_[13], worldV_[14], worldV_[0], worldV_[5], worldV_[10], viewV_[12], viewV_[13], viewV_[14], projV_[0], projV_[5], projV_[10], projV_[11], projV_[12], projV_[13], projV_[14], projV_[15], vpX_, vpY_, vpW_, vpH_, depthMin_, depthMax_);
                    fprintf(stderr, "[fl] pass: fvf=0x%x tex0=%p(fmt %d) tex1=%p(fmt %d) c0=%d/%d/%d a0=%d c1=%d/%d/%d a1=%d lit=%d n=%u\n", fvf, stageTex_[0], fmtOf(stageTex_[0]), stageTex_[1], fmtOf(stageTex_[1]),
                            stageState_[0][1], stageState_[0][2], stageState_[0][3], stageState_[0][4], stageState_[1][1], stageState_[1][2], stageState_[1][3], stageState_[1][4], renderState_[137], count);
                    fprintf(stderr, "        filters st0: mag=%d min=%d mip=%d aniso=%d lodbias=%d addrU=%d addrV=%d | st1: mag=%d min=%d mip=%d\n", stageState_[0][16], stageState_[0][17], stageState_[0][18], stageState_[0][21], stageState_[0][19], stageState_[0][13], stageState_[0][14], stageState_[1][16], stageState_[1][17], stageState_[1][18]);
                    fprintf(stderr, "        blend=%d src=%d dst=%d atest=%d(func %d ref %d) zen=%d zfunc=%d zwrite=%d cull=%d lit=%d tfactor=%08x\n", renderState_[27], renderState_[19], renderState_[20], renderState_[15], renderState_[25], renderState_[24], renderState_[7], renderState_[23], renderState_[14], renderState_[22], renderState_[137], (unsigned)renderState_[60]);
                }
            }
            if (getenv("FL_TRACE_QUADS") && fvf == 0x1c4)
            {
                static int n = 0;
                if (n++ < 6)
                {
                    const float* f = (const float*)verts;
                    const uint32_t* u = (const uint32_t*)verts;
                    fprintf(stderr, "[fl] quad: nv=%u ni=%u v0=(%g %g %g rhw=%g col=%08x uv=%g,%g) v2=(%g %g) tex=%p ablend=%d src=%d dst=%d cop=%d ca1=%d ca2=%d aop=%d vp=%d,%d,%d,%d\n", nverts, count, f[0], f[1], f[2], f[3], u[4], f[6], f[7], f[14], f[15], stageTex_[0], renderState_[27], renderState_[19], renderState_[20], stageState_[0][1], stageState_[0][2], stageState_[0][3], stageState_[0][4], vpX_, vpY_, vpW_, vpH_);
                }
            }
            if (getenv("FL_TRACE_DRAWS") && drawCalls_ < 8)
            {
                const float* f = (const float*)verts;
                fprintf(stderr, "[fl] draw prim=%u fvf=0x%x nv=%u ni=%u stride=%u v0=(%g %g %g %g) tex0=%p zen=%d blend=%d cop0=%d carg=%d,%d\n", prim, fvf, nverts, count, L.stride,
                        f[0], f[1], f[2], L.rhw ? f[3] : 0.f, stageTex_[0], renderState_[7], renderState_[27], stageState_[0][1], stageState_[0][2], stageState_[0][3]);
            }
            ++drawCalls_;
            viewMarker_ = false;   // RP8: committing the transforms at draw time clears the 'view set by SetModelview' marker
            garbage_.push_back(vb);
            if (indexed) garbage_.push_back(ib);
            return GR_OK;
        }

        // Generic vertex copier for the engine's "source descriptor" streams (position / normal / diffuse / 2 texcoords).
        static void ConvertVertices(unsigned char* dst, unsigned fvf, const FRpVertexSrcDesc* src, unsigned start, unsigned count)
        {
            const fldraw::FvfLayout L = fldraw::MakeLayout(fvf);
            auto at = [&](const FRpVertexStream& s, unsigned v) -> const unsigned char*
            {
                if (!s.Base) return nullptr;
                const unsigned i = ((src->Flags & 4) && s.IndexArray) ? s.IndexArray[v] : v;
                return s.Base + (size_t)i * (s.Stride ? s.Stride : s.ByteSize);
            };
            for (unsigned v = start; v < start + count; ++v)
            {
                unsigned char* o = dst + (size_t)v * L.stride;
                if (const unsigned char* p = at(src->Position, v)) memcpy(o, p, L.rhw ? 16 : 12);
                if (L.normal >= 0) if (const unsigned char* p = at(src->Normal, v)) memcpy(o + L.normal, p, 12);
                if (L.diffuse >= 0) if (const unsigned char* p = at(src->Diffuse, v)) memcpy(o + L.diffuse, p, 4);
                for (int t = 0; t < 2; ++t)
                    if (L.uv[t] >= 0) if (const unsigned char* p = at(src->Tex[t], v)) memcpy(o + L.uv[t], p, 8);
            }
        }

        // ------------------------------------------------------------ draw entry points
        GENRESULT DACOM_API DrawPrimitive(uint32_t prim, unsigned fvf, const void* verts, unsigned count, void*) override
        {
            if (count == 0) return GR_GENERIC;
            return Submit(prim, fvf, (const unsigned char*)verts, count, nullptr, 0);
        }
        GENRESULT DACOM_API DrawIndexedPrimitiveUp(uint32_t prim, unsigned fvf, const void* verts, unsigned vcount, const void* indices, unsigned icount, uint32_t) override
        {
            if (icount == 0) return GR_GENERIC;
            return Submit(prim, fvf, (const unsigned char*)verts, vcount, (const uint16_t*)indices, icount);
        }
        GENRESULT DACOM_API DrawPrimitiveVb(uint32_t prim, void* vb, unsigned start, unsigned count, void*) override
        {
            FBuffer* b = (FBuffer*)vb;
            if (!b || !vbs_.count(b) || count == 0) { WarnOnce("DrawPrimitiveVb dropped: bad VB handle", b ? 1 : 0); return GR_GENERIC; }
            const unsigned stride = fldraw::MakeLayout(b->fvf).stride;
            if (!stride || (size_t)(start + count) * stride > b->bytes.size()) { WarnOnce("DrawPrimitiveVb dropped: vertex range outside the VB", (int)b->fvf); return GR_GENERIC; }
            return Submit(prim, b->fvf, b->bytes.data() + (size_t)start * stride, count, nullptr, 0);
        }
        GENRESULT DACOM_API DrawIndexedPrimitiveVb(uint32_t prim, void* vb, unsigned base, unsigned nverts, const void* indices, unsigned icount, uint32_t) override
        {
            FBuffer* b = (FBuffer*)vb;
            if (!b || !vbs_.count(b) || icount == 0) { WarnOnce("DrawIndexedPrimitiveVb dropped: bad VB handle", b ? 1 : 0); return GR_GENERIC; }
            const unsigned stride = fldraw::MakeLayout(b->fvf).stride;
            if (!stride || (size_t)(base + nverts) * stride > b->bytes.size()) { WarnOnce("DrawIndexedPrimitiveVb dropped: vertex range outside the VB", (int)b->fvf); return GR_GENERIC; }
            return Submit(prim, b->fvf, b->bytes.data() + (size_t)base * stride, nverts, (const uint16_t*)indices, icount);
        }
        GENRESULT DACOM_API DrawIndexedPrimitive(uint32_t prim, unsigned minIndex, unsigned numVerts, unsigned startIndex, unsigned count) override
        {
            if (!selVb_ || !selIb_ || count == 0 || (size_t)(startIndex + count) > selIb_->idx.size())
            {
                WarnOnce("DrawIndexedPrimitive dropped: no VB/IB selected or index range outside the IB", selVb_ ? (selIb_ ? 2 : 1) : 0);
                return GR_GENERIC;
            }
            const unsigned stride = fldraw::MakeLayout(selVb_->fvf).stride;
            const size_t firstVert = (size_t)selIbBase_ + minIndex;
            if (!stride || (firstVert + numVerts) * stride > selVb_->bytes.size())
            {
                WarnOnce("DrawIndexedPrimitive dropped: vertex range outside the VB", (int)selVb_->fvf);
                return GR_GENERIC;
            }
            std::vector<uint16_t> rebased(selIb_->idx.begin() + startIndex, selIb_->idx.begin() + startIndex + count);
            if (minIndex) for (auto& i : rebased) i = (uint16_t)(i - minIndex);
            return Submit(prim, selVb_->fvf, selVb_->bytes.data() + firstVert * stride, numVerts, rebased.data(), count);
        }

        // ------------------------------------------------------------ vertex / index copies
        GENRESULT DACOM_API CopyVertexData(void* dest, unsigned fvf, const FRpVertexSrcDesc* src, unsigned start, unsigned count) override
        {
            ConvertVertices((unsigned char*)dest, fvf, src, start, count);
            return GR_OK;
        }
        GENRESULT DACOM_API CopyVertices(void* h, unsigned* outOffset, const FRpVertexSrcDesc* src, unsigned start, unsigned count) override
        {
            if (!h || !src) return (GENRESULT)0xFFFFFFFF;
            FBuffer* b;
            unsigned offset = outOffset ? *outOffset : 0;
            if (h == (void*)(intptr_t)-1)
            {
                FBuffer*& pool = poolVb_[src->SrcFvf];
                if (!pool) { pool = new FBuffer(); pool->fvf = src->SrcFvf; vbs_.insert(pool); }
                b = pool; offset = b->count; b->count += count;
                selVb_ = pool;
            }
            else { b = (FBuffer*)h; if (!vbs_.count(b)) return (GENRESULT)0xFFFFFFFF; }
            const unsigned stride = fldraw::MakeLayout(b->fvf).stride;
            if ((size_t)(offset + count) * stride > b->bytes.size()) b->bytes.resize((size_t)(offset + count) * stride + 4096);
            // ConvertVertices addresses vertices [start, start+count) relative to dst; shift so they land at `offset`.
            ConvertVertices(b->bytes.data() + (ptrdiff_t)(offset * stride) - (ptrdiff_t)(start * stride), b->fvf, src, start, count);
            if (outOffset) *outOffset = offset;
            return GR_OK;
        }
        GENRESULT DACOM_API CopyIndices(void* h, unsigned* outOffset, const unsigned short* srcData, unsigned count) override
        {
            if (!h || !srcData) return (GENRESULT)0x80004005;
            FIndexBuf* b = h == (void*)(intptr_t)-1 ? ScratchIb() : (FIndexBuf*)h;
            if (!ibs_.count(b)) return (GENRESULT)0x80004005;
            unsigned offset = outOffset ? *outOffset : 0;
            if (b == scratchIb_) { offset = scratchUsed_; scratchUsed_ += count; }
            if (b->idx.size() < (size_t)offset + count) b->idx.resize((size_t)offset + count + 1024);
            memcpy(b->idx.data() + offset, srcData, (size_t)count * 2);
            if (outOffset) *outOffset = offset;
            return GR_OK;
        }
        GENRESULT DACOM_API ResizeVb(void* h, unsigned fvf, unsigned count) override
        {
            FBuffer* b = (FBuffer*)h; if (!b || !vbs_.count(b)) return (GENRESULT)0x80004005;
            b->fvf = fvf; b->bytes.resize((size_t)fldraw::MakeLayout(fvf).stride * count); return GR_OK;
        }
        GENRESULT DACOM_API ResizeIb(void* h, unsigned count) override
        {
            FIndexBuf* b = (FIndexBuf*)h; if (!b || !ibs_.count(b)) return (GENRESULT)0x80004005;
            b->count = count; b->idx.resize(count); return GR_OK;
        }

        // ------------------------------------------------------------ device life
        GENRESULT DACOM_API Startup(const char*) override { started_ = true; return GR_OK; }
        GENRESULT DACOM_API ShutdownPipeline() override { DestroyGpuResources(); started_ = false; return GR_OK; }
        GENRESULT DACOM_API SetPipelineState(int id, int v) override { if (id >= 0 && id < 16) pipeState_[id] = v; return GR_OK; }
        GENRESULT DACOM_API GetPipelineState(int id, int* out) override { if (id < 0 || id >= 16 || !out) return GR_INVALID_PARMS; *out = pipeState_[id]; return GR_OK; }
        GENRESULT DACOM_API GetDeviceInfo(void* out) override { memset(out, 0, 0x520); return GR_OK; }
        GENRESULT DACOM_API QueryDeviceAbility(unsigned idx, uint32_t* out) override
        {
            if (idx > 0x14 || !out) return GR_INVALID_PARMS;
            *out = ability_[idx];
            return GR_OK;
        }
        GENRESULT DACOM_API GetDisplayModeCount(unsigned* out) override { *out = 1; return GR_OK; }
        GENRESULT DACOM_API GetDisplayModeEnum(unsigned idx, void* out) override
        {
            uint32_t* r = static_cast<uint32_t*>(out);
            r[0] = idx; r[1] = 1024; r[2] = 768; r[3] = 60; r[4] = 0;
            return GR_OK;
        }
        GENRESULT DACOM_API SelectMode(int*, unsigned* idx) override { *idx = 0; return GR_OK; }
        GENRESULT DACOM_API CreateBuffers(HWND hwnd, int* mode, int* outMode) override
        {
            fprintf(stderr, "[fl] render: CreateBuffers hwnd=%p started=%d\n", (void*)hwnd, (int)started_);
            if (!started_) { fprintf(stderr, "[fl] render: CreateBuffers before Startup\n"); return GR_GENERIC; }
            hwnd_ = hwnd;
            memcpy(mode_, mode, sizeof(mode_));
            if (outMode) memcpy(outMode, mode_, sizeof(mode_));
            fl::render::IWindowSurface* s = fl::render::GetWindowSurface();
            if (!s || !hwnd) { fprintf(stderr, "[fl] render: no window surface\n"); return GR_GENERIC; }
            if (!sgReady_)
            {
                if (!s->CreateContext(hwnd)) { fprintf(stderr, "[fl] render: surface creation failed\n"); return GR_GENERIC; }
                sg_desc desc = {};
                desc.logger.func = [](const char* tag, uint32_t level, uint32_t id, const char* msg, uint32_t line, const char* file, void*) { fprintf(stderr, "[sokol:%s] lvl=%u id=%u %s (%s:%u)\n", tag ? tag : "", level, id, msg ? msg : "", file ? file : "", line); };
#if defined(__APPLE__)
                metal_ = flmetal::Create(s->NativeLayer(hwnd));
                if (!metal_) { fprintf(stderr, "[fl] render: no Metal device/layer\n"); return GR_GENERIC; }
                desc.environment.metal.device = flmetal::Device(metal_);
#endif
                desc.buffer_pool_size = 16384; desc.image_pool_size = 4096; desc.view_pool_size = 4096; desc.pipeline_pool_size = 1024; desc.sampler_pool_size = 256;
                desc.environment.defaults.color_format = SG_PIXELFORMAT_BGRA8;
                desc.environment.defaults.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
                desc.environment.defaults.sample_count = 1;
                sg_setup(&desc);
                sgReady_ = sg_isvalid();
                if (!sgReady_) { fprintf(stderr, "[fl] render: sg_setup failed\n"); return GR_GENERIC; }
                InitGpu();
            }
            clearPending_ = true;
            return GR_OK;
        }
        GENRESULT DACOM_API GetModeBlob(void* adapter, void* mode10) override
        {
            if (adapter) memset(adapter, 0, 0x520);
            if (mode10) memcpy(mode10, mode_, sizeof(mode_));
            return GR_OK;
        }
        GENRESULT DACOM_API DestroyGpuResources() override { EndPassIfOpen(); return GR_OK; }
        GENRESULT DACOM_API ClearBuffers(uint32_t, const void*) override { clearPending_ = true; return GR_OK; }
        GENRESULT DACOM_API SwapBuffers() override
        {
            if (!sgReady_) return GR_OK;
            // Always present a defined frame: with no pass open the drawable would hold garbage (purple flashes).
            if (!passOpen_) { clearPending_ = true; BeginPassIfNeeded(); }
            EndPassIfOpen();
            sg_commit();
            for (sg_buffer b : garbage_) sg_destroy_buffer(b);
            garbage_.clear();
            for (auto& kv : poolVb_) kv.second->count = 0;
            scratchUsed_ = 0;
            if (getenv("FL_TRACE_RENDER") && drawCalls_) { fprintf(stderr, "[fl] render: %u draw calls this frame\n", drawCalls_); }
            drawCalls_ = 0;
            static int frames = 0;
            if (getenv("FL_TRACE_RENDER") && (++frames % 60) == 1)
                fprintf(stderr, "[fl] render: frame %d drawable=%p size=%dx%d\n", frames, drawable_, winW_, winH_);
            if (fl::render::IWindowSurface* s = fl::render::GetWindowSurface()) s->Present(hwnd_);
            return GR_OK;
        }
        GENRESULT DACOM_API BeginScene() override { inScene_ = true; BeginPassIfNeeded(); return GR_OK; }
        GENRESULT DACOM_API EndScene() override { inScene_ = false; return GR_OK; }   // pass closes at SwapBuffers
        GENRESULT DACOM_API VerifyState() override { return GR_OK; }

        // ------------------------------------------------------------ viewport / window / matrices
        static bool TraceCam() { static const bool on = getenv("FL_TRACE_CAM") != nullptr; return on; }
        GENRESULT DACOM_API SetViewportRect(int x, int y, int w, int h) override { if (TraceCam()) fprintf(stderr, "[cam] viewport %d,%d %dx%d\n", x, y, w, h); vpX_ = x; vpY_ = y; vpW_ = w; vpH_ = h; return GR_OK; }
        GENRESULT DACOM_API GetViewportRect(int* x, int* y, int* w, int* h) override { *x = vpX_; *y = vpY_; *w = vpW_; *h = vpH_; return GR_OK; }
        GENRESULT DACOM_API SetDepthRange(float a, float b) override { depthMin_ = a; depthMax_ = b; return GR_OK; }
        GENRESULT DACOM_API GetDepthRange(float* a, float* b) override { *a = depthMin_; *b = depthMax_; return GR_OK; }
        GENRESULT DACOM_API SetWindow(HWND h, int x, int y, int w, int hh) override { hwnd_ = h; winX_ = x; winY_ = y; winW_ = w; winH_ = hh; return GR_OK; }
        GENRESULT DACOM_API GetWindow(HWND* h, int* l, int* r, int* w, int* hh) override { *h = hwnd_; *l = winX_; *r = winX_ + winW_; *w = winW_; *hh = winH_; return GR_OK; }
        GENRESULT DACOM_API SetWorld(const float* m) override { memcpy(worldRaw_, m, sizeof(worldRaw_)); fldraw::FromAffine(worldV_, m, false); return GR_OK; }
        GENRESULT DACOM_API GetWorld(float* m) override { memcpy(m, worldRaw_, sizeof(worldRaw_)); return GR_OK; }
        GENRESULT DACOM_API SetWorldN(int idx, const float* m) override { if (idx == 0) return SetWorld(m); return GR_OK; }
        GENRESULT DACOM_API SetView(const float* m) override { memcpy(viewRaw_, m, sizeof(viewRaw_)); fldraw::FromAffine(viewV_, m, true); return GR_OK; }
        GENRESULT DACOM_API GetView(float* m) override { memcpy(m, viewRaw_, sizeof(viewRaw_)); return GR_OK; }
        GENRESULT DACOM_API SetModelview(const float* m) override
        {
            memcpy(modelviewRaw_, m, sizeof(modelviewRaw_));
            const bool lightsOff = pipeState_[7] == 0;
            fldraw::FromAffine(worldV_, m, !lightsOff);
            if (!viewMarker_)
            {
                const float id[12] = {1,0,0, 0,1,0, 0,0,1, 0,0,0};
                fldraw::FromAffine(viewV_, id, lightsOff);
                viewMarker_ = true;
            }
            return GR_OK;
        }
        GENRESULT DACOM_API GetModelview(float* m) override { memcpy(m, modelviewRaw_, sizeof(modelviewRaw_)); return GR_OK; }
        GENRESULT DACOM_API SetProjection(const float* m) override { if (TraceCam()) fprintf(stderr, "[cam] projection [%g %g %g %g | %g %g %g %g]\n", m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7]); memcpy(proj_, m, sizeof(proj_)); fldraw::FromTransposed(projV_, m); return GR_OK; }
        GENRESULT DACOM_API GetProjection(float* m) override { memcpy(m, proj_, sizeof(proj_)); return GR_OK; }
        GENRESULT DACOM_API SetLookat(float ex, float ey, float ez, float ax, float ay, float az, float ux, float uy, float uz) override
        {
            auto norm = [](float* v) { float l = sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]); if (l > 0) { v[0] /= l; v[1] /= l; v[2] /= l; } };
            float f[3] = {ax - ex, ay - ey, az - ez}; norm(f);
            float z[3] = {-f[0], -f[1], -f[2]};
            float x[3] = {uy * z[2] - uz * z[1], uz * z[0] - ux * z[2], ux * z[1] - uy * z[0]}; norm(x);
            float y[3] = {z[1] * x[2] - z[2] * x[1], z[2] * x[0] - z[0] * x[2], z[0] * x[1] - z[1] * x[0]}; norm(y);
            float m[12] = {x[0], x[1], x[2], y[0], y[1], y[2], z[0], z[1], z[2], 0, 0, 0};
            m[9]  = -(x[0] * ex + x[1] * ey + x[2] * ez);
            m[10] = -(y[0] * ex + y[1] * ey + y[2] * ez);
            m[11] = -(z[0] * ex + z[1] * ey + z[2] * ez);
            if (pipeState_[7] != 0)
            {
                fldraw::FromAffine(worldV_, m, true);
                if (!viewMarker_) { const float id[12] = {1,0,0, 0,1,0, 0,0,1, 0,0,0}; fldraw::FromAffine(viewV_, id, false); viewMarker_ = true; }
            }
            else fldraw::FromAffine(viewV_, m, true);
            return GR_OK;
        }
        GENRESULT DACOM_API SetOrtho(float l, float r, float b, float t, float n, float f) override
        {
            if (TraceCam()) fprintf(stderr, "[cam] ortho l=%g r=%g b=%g t=%g n=%g f=%g\n", l, r, b, t, n, f);
            memset(projV_, 0, sizeof(projV_));
            const double w = (double)r - l, h = (double)t - b, d = (double)f - n;
            projV_[0] = (float)(2.0 / w); projV_[5] = (float)(2.0 / h); projV_[10] = (float)(2.0 / d);
            projV_[12] = (float)(-((double)r + l) / w); projV_[13] = (float)(-((double)b + t) / h); projV_[14] = (float)(-(double)n / d); projV_[15] = 1.0f;
            return GR_OK;
        }
        GENRESULT DACOM_API SetPerspective(float fov, float aspect, float n, float f) override
        {
            if (TraceCam()) fprintf(stderr, "[cam] perspective fov=%g aspect=%g near=%g far=%g\n", fov, aspect, n, f);
            if (n <= 0.0f || f <= 0.0f || f == n) return GR_GENERIC;
            const float cot = (float)(1.0 / tan(3.141592653589793 / 180.0 * (double)fov));
            const float inv = 1.0f / (f - n);
            memset(projV_, 0, sizeof(projV_));
            projV_[0] = cot / aspect; projV_[5] = cot; projV_[10] = f * inv; projV_[11] = 1.0f; projV_[14] = -f * n * inv;
            return GR_OK;
        }

        // ------------------------------------------------------------ lights / material
        GENRESULT DACOM_API SetLight(void* h, const D3DLIGHT8* l) override { lights_[h] = *l; return GR_OK; }
        GENRESULT DACOM_API DestroyLight(void* h) override { lights_.erase(h); lightEnabled_.erase(h); return GR_OK; }
        GENRESULT DACOM_API GetLight(void* h, D3DLIGHT8* l) override { auto it = lights_.find(h); if (it == lights_.end()) return GR_INVALID_PARMS; *l = it->second; return GR_OK; }
        GENRESULT DACOM_API SetLightEnable(void* h, int e) override { lightEnabled_[h] = e; return GR_OK; }
        // AddLight/UpdateLight: the handle is the caller's own D3DLIGHT8 (it doubles as the record's data, as in RP8); copy it now.
        GENRESULT DACOM_API AddLight(void* h) override { if (h) lights_[h] = *static_cast<const D3DLIGHT8*>(h); return GR_OK; }
        GENRESULT DACOM_API UpdateLight(void* h) override { if (h) lights_[h] = *static_cast<const D3DLIGHT8*>(h); return GR_OK; }
        GENRESULT DACOM_API RemoveLight(void* h) override { lights_.erase(h); lightEnabled_.erase(h); return GR_OK; }
        GENRESULT DACOM_API GetLightEnable(void* h, int* e) override { *e = lightEnabled_.count(h) ? lightEnabled_[h] : 0; return GR_OK; }
        GENRESULT DACOM_API SetMaterial(const D3DMATERIAL8* m) override { material_ = *m; return GR_OK; }
        GENRESULT DACOM_API GetMaterial(D3DMATERIAL8* m) override { *m = material_; return GR_OK; }

        // ------------------------------------------------------------ render / stage state
        GENRESULT DACOM_API ResetRenderStatesToDefaults() override
        {
            memset(renderState_, 0, sizeof(renderState_)); memset(stageState_, 0, sizeof(stageState_));
            renderState_[7] = 1; renderState_[9] = 2; renderState_[14] = 1; renderState_[19] = 2; renderState_[20] = 1;
            renderState_[22] = 3; renderState_[23] = 4; renderState_[25] = 8; renderState_[60] = (int)0xFFFFFFFF;
            renderState_[137] = 1; renderState_[168] = 0xF;
            renderState_[141] = 1; renderState_[145] = 1; renderState_[146] = 2;   // COLORVERTEX, DIFFUSE from COLOR1, SPECULAR from COLOR2
            material_.Diffuse = {1, 1, 1, 1}; material_.Ambient = {1, 1, 1, 1};
            for (int i = 0; i < 8; ++i)
            {
                stageState_[i][1] = i == 0 ? 4 : 1; stageState_[i][2] = 2; stageState_[i][3] = 1;
                stageState_[i][4] = i == 0 ? 2 : 1; stageState_[i][5] = 2; stageState_[i][6] = 1;
                stageState_[i][11] = 0; stageState_[i][13] = 1; stageState_[i][14] = 1; stageState_[i][16] = 2; stageState_[i][17] = 2; stageState_[i][18] = 1;
            }
            return GR_OK;
        }
        // Anything the engine asks for that this backend does not render yet is reported once, never silently dropped.
        static void WarnOnce(const char* what, int a = 0)
        {
            static std::set<std::string> seen;
            char key[96]; snprintf(key, sizeof(key), "%s %d", what, a);
            if (seen.insert(key).second) fprintf(stderr, "[fl] render: NOT IMPLEMENTED: %s (%d)\n", what, a);
        }
        GENRESULT DACOM_API SetRenderState(D3DRENDERSTATETYPE s, int v) override
        {
            if ((unsigned)s >= 256) return GR_OK;
            renderState_[s] = v;
            if (v)
                switch ((int)s)
                {
                case 28: WarnOnce("fog enable", v); break;
                case 29: WarnOnce("specular enable", v); break;
                case 52: WarnOnce("stencil enable", v); break;
                case 48: WarnOnce("range fog", v); break;
                case 154: case 155: WarnOnce("point sprite/size state", (int)s); break;
                case 171: WarnOnce("blend op", v); break;
                case 26: case 27 + 100: break;
                }
            if ((int)s == 15 && v) {} // alpha test is implemented
            return GR_OK;
        }
        GENRESULT DACOM_API GetRenderState(D3DRENDERSTATETYPE s, int* out) override { if ((unsigned)s >= 256) return GR_INVALID_PARMS; *out = renderState_[s]; return GR_OK; }
        GENRESULT DACOM_API SetTextureStageState(int st, int t, int v) override
        {
            if ((unsigned)st < 8 && (unsigned)t < 64)
            {
                stageState_[st][t] = v;
                if (st < 2 && (t == 1 || t == 4) && v > 16) WarnOnce("texture op (bump/dot3/multiply-add/lerp)", v);
                if (st < 2 && t == 24 && v) WarnOnce("texture coordinate transform", v);
                if (st < 2 && t == 11 && (v & ~0xFFFF)) WarnOnce("texture coordinate generation (camera-space/reflection)", v >> 16);
                if (st >= 2 && t == 1 && v > 1) WarnOnce("more than two texture stages", st);
            }
            return GR_OK;
        }
        GENRESULT DACOM_API GetTextureStageState(int st, int t, int* out) override { if ((unsigned)st >= 8 || (unsigned)t >= 64) return GR_INVALID_PARMS; *out = stageState_[st][t]; return GR_OK; }
        GENRESULT DACOM_API SetTextureStageTransform(int st, const float* m) override { if ((unsigned)st < 8) memcpy(texXform_[st], m, sizeof(texXform_[st])); return GR_OK; }
        GENRESULT DACOM_API GetTextureStageTransform(int st, float* m) override { if ((unsigned)st >= 8) return GR_INVALID_PARMS; memcpy(m, texXform_[st], sizeof(texXform_[st])); return GR_OK; }
        GENRESULT DACOM_API SetTextureStageTexture(int st, void* h) override { if ((unsigned)st < 8) stageTex_[st] = h; return GR_OK; }
        GENRESULT DACOM_API GetTextureStageTexture(int st, void** h) override { if ((unsigned)st >= 8) return GR_INVALID_PARMS; *h = stageTex_[st]; return GR_OK; }

        // ------------------------------------------------------------ textures (CPU side)
        GENRESULT DACOM_API CreateTexture(int w, int h, unsigned* fmt, unsigned levels, unsigned, void** out) override
        {
            FTexture* t = new FTexture();
            t->width = (unsigned)w; t->height = (unsigned)h; t->format = fmt ? *fmt : 7;
            // Ids 0 and 15..21 are texture *classes* (DAOP, DAOT, DAAA, DAAL, DAA1, DAA4, DAA8) the device resolves to
            // a concrete storage format; 32-bit A8R8G8B8 is always available here. 9 is unused.
            if (t->format == 0 || t->format == 9 || t->format > 14) t->format = 7;
            if (fmt) *fmt = t->format;
            t->levels = levels ? levels : 1;
            unsigned lw = t->width, lh = t->height;
            for (unsigned l = 0; l < t->levels; ++l)
            {
                t->data.emplace_back(LevelBytes(t->format, lw, lh));
                lw = lw > 1 ? lw / 2 : 1; lh = lh > 1 ? lh / 2 : 1;
            }
            textures_.insert(t);
            *out = t;
            return GR_OK;
        }
        GENRESULT DACOM_API DestroyTexture(void* h) override { FTexture* t = (FTexture*)h; if (textures_.erase(t)) { FreeGpuTexture(t); for (auto& st : stageTex_) if (st == h) st = nullptr; delete t; } return GR_OK; }
        GENRESULT DACOM_API IsTexture(void* h) override { return textures_.count((FTexture*)h) ? GR_OK : GR_GENERIC; }
        GENRESULT DACOM_API LockTexture(void* h, int level, void* out) override
        {
            FTexture* t = (FTexture*)h;
            if (!t) return (GENRESULT)0xFFFFFFFE;
            if (!textures_.count(t) || (unsigned)level >= t->levels) return GR_GENERIC;
            unsigned lw = t->width >> level, lh = t->height >> level;
            if (!lw) lw = 1; if (!lh) lh = 1;
            uint32_t* o = static_cast<uint32_t*>(out);
            o[0] = lw; o[1] = lh;
            o[2] = IsDxt(t->format) ? LevelBytes(t->format, lw, 1) : lw * (BytesPerPixel(t->format) ? BytesPerPixel(t->format) : 4);
            o[3] = fl::PtrToHandle(t->data[level].data());   // 32-bit slot in the retail lock record
            t->dirty = true;
            o[4] = t->format;
            return GR_OK;
        }
        GENRESULT DACOM_API UnlockTexture(void* h, int) override { FTexture* t = (FTexture*)h; if (t && textures_.count(t)) t->dirty = true; return GR_OK; }
        GENRESULT DACOM_API GetTextureFormat(void* h, int* out) override { FTexture* t = (FTexture*)h; if (!t) return GR_INVALID_PARMS; *out = (int)t->format; return GR_OK; }
        GENRESULT DACOM_API GetTextureDim(void* h, unsigned* w, unsigned* hh, int* lv) override { FTexture* t = (FTexture*)h; if (!t) return GR_INVALID_PARMS; *w = t->width; *hh = t->height; *lv = (int)t->levels; return GR_OK; }
        GENRESULT DACOM_API SetTextureLevelData(void* h, int level, int sw, int sh, uint32_t pitch, uint32_t* srcFormat, const void* px, void*, void* palette) override
        {
            FTexture* t = (FTexture*)h;
            if (!t || (unsigned)level >= t->levels || !px) return GR_INVALID_PARMS;
            t->dirty = true;
            auto& dst = t->data[level];
            const uint32_t sf = srcFormat ? *srcFormat : t->format;
            if (IsDxt(t->format) || IsDxt(sf))
            {   // compressed -> compressed: raw copy, bounded by both sizes
                size_t n = LevelBytes(sf, (unsigned)sw, (unsigned)sh);
                if (n > dst.size()) n = dst.size();
                memcpy(dst.data(), px, n);
                return GR_OK;
            }
            if (getenv("FL_TRACE_TEX")) { static std::set<std::pair<unsigned, unsigned>> seenFmt; if (seenFmt.insert({sf, t->format}).second) fprintf(stderr, "[fl] tex upload: src format %u -> texture format %u (%dx%d level %d)\n", sf, t->format, sw, sh, level); }
            const unsigned sb = BytesPerPixel(sf) ? BytesPerPixel(sf) : 4, db = BytesPerPixel(t->format) ? BytesPerPixel(t->format) : 4;
            const unsigned lw = (t->width >> level) ? (t->width >> level) : 1, lh = (t->height >> level) ? (t->height >> level) : 1;
            const unsigned cw = (unsigned)sw < lw ? (unsigned)sw : lw, ch = (unsigned)sh < lh ? (unsigned)sh : lh;
            const size_t srcPitch = pitch ? pitch : (size_t)sw * sb;
            for (unsigned y = 0; y < ch; ++y)
            {
                const unsigned char* sp = (const unsigned char*)px + y * srcPitch;
                unsigned char* d = dst.data() + (size_t)y * lw * db;
                if (sf == t->format) { memcpy(d, sp, (size_t)cw * db); continue; }
                if (t->format == 7 && sf == 1 && palette)
                {   // P8: RP8 reads the palette as 256 RGB triples (3 bytes per entry) with flags 0xff
                    const unsigned char* pal = (const unsigned char*)palette;
                    for (unsigned x = 0; x < cw; ++x)
                    {
                        const unsigned char* e = pal + 3u * sp[x];
                        d[x * 4 + 0] = e[2]; d[x * 4 + 1] = e[1]; d[x * 4 + 2] = e[0]; d[x * 4 + 3] = 255;
                    }
                    continue;
                }
                if (t->format == 7 && sf >= 1 && sf <= 8) { fldraw::ToBgra8(sf, cw, 1, sp, d); continue; }
                for (unsigned x = 0; x < cw; ++x)          // anything else: widen/narrow bytewise
                {
                    memset(d + (size_t)x * db, 0xFF, db);
                    memcpy(d + (size_t)x * db, sp + (size_t)x * sb, sb < db ? sb : db);
                }
            }
            return GR_OK;
        }

        // ------------------------------------------------------------ IRPTexture (image-file helpers)
        // GetImageInfo: DDS headers only (what the retail texture library asks for); other containers are
        // loaded by the library's own TGA path.
        GENRESULT DACOM_API GetImageInfo(_D3DXIMAGE_INFO* info, uint32_t size, const void* data) override
        {
            const uint32_t* H = static_cast<const uint32_t*>(data);
            if (!data || size < 0x80 || H[0] != 0x20534444u /* "DDS " */) return GR_GENERIC;
            memset(info, 0, sizeof(*info));
            info->Height = H[3];
            info->Width = H[4];
            info->Depth = 1;
            info->MipLevels = (H[2] & 0x20000) && H[7] ? H[7] : 1;   // DDSD_MIPMAPCOUNT
            if (H[0x50 / 4] & 4) info->Format = (D3DFORMAT)H[0x54 / 4];   // FourCC (DXTn)
            else
            {   // uncompressed: the D3D format follows from the pixel format's bit count and channel masks
                const uint32_t bits = H[0x58 / 4], rm = H[0x5C / 4], gm = H[0x60 / 4], am = H[0x68 / 4];
                (void)rm;
                uint32_t f = 21;                                    // A8R8G8B8
                if (bits == 32) f = (H[0x50 / 4] & 1) && am ? 21 : 22;   // A8R8G8B8 / X8R8G8B8
                else if (bits == 24) f = 20;                        // R8G8B8
                else if (bits == 16) f = gm == 0x07E0 ? 23 : (am == 0x8000 ? 25 : (am == 0xF000 ? 26 : 24));   // R5G6B5 / A1R5G5B5 / A4R4G4B4 / X1R5G5B5
                else if (bits == 8) f = 41;                         // P8
                info->Format = (D3DFORMAT)f;
            }
            info->ResourceType = (D3DRESOURCETYPE)((H[0x70 / 4] & 0x200) ? 5 : 3);   // cube map : texture
            info->ImageFileFormat = (D3DXIMAGE_FILEFORMAT)4;                         // D3DXIFF_DDS
            return GR_OK;
        }
        // Whole-file loaders are not needed by the engine paths seen so far; a 1x1 placeholder keeps callers alive.
        GENRESULT DACOM_API LoadTexture(IFileSystem*, const char* name, void** out) override
        {
            fprintf(stderr, "[fl] render: LoadTexture('%s') -> placeholder\n", name ? name : "?");
            unsigned pf = 7; return CreateTexture(1, 1, &pf, 1, 0, out);
        }
        GENRESULT DACOM_API LoadCubemap(IFileSystem*, const char* name, void** out) override
        {
            fprintf(stderr, "[fl] render: LoadCubemap('%s') -> placeholder\n", name ? name : "?");
            unsigned pf = 7; return CreateTexture(1, 1, &pf, 1, 0, out);
        }

        // ------------------------------------------------------------ vertex buffer manager / VBs / IBs (CPU memory)
        GENRESULT DACOM_API VbMgrInit(void*, void*, void*, void*) override { return GR_OK; }
        GENRESULT DACOM_API VbMgrReleaseAll() override { return GR_OK; }
        GENRESULT DACOM_API LockVertexMemory(unsigned fvf, unsigned count, FVertexMemoryLock* out) override
        {
            const unsigned stride = fldraw::MakeLayout(fvf).stride;
            FBuffer*& pool = poolVb_[fvf];
            if (!pool) { pool = new FBuffer(); pool->fvf = fvf; vbs_.insert(pool); }
            const unsigned offset = pool->count;          // FBuffer::count is the pool's bump cursor
            if (pool->bytes.size() < (size_t)(offset + count) * stride) pool->bytes.resize((size_t)(offset + count) * stride + 4096);
            pool->count = offset + count;
            selVb_ = pool;   // RP8: LockVertexMemory selects the pool stream for the draw that follows
            out->Listing.VertexBuffer = pool; out->Listing.Fvf = fvf; out->Listing.BaseVertex = offset;
            out->Vertices = pool->bytes.data() + (size_t)offset * stride;
            return GR_OK;
        }
        GENRESULT DACOM_API UnlockVertexMemory(FVertexMemoryLock*) override { return GR_OK; }
        GENRESULT DACOM_API UnsupportedManagerOperation() override { return (GENRESULT)0xFFFFFFF9; }

        GENRESULT DACOM_API CreateVb(unsigned fvf, unsigned count, unsigned** out, unsigned) override
        {
            FBuffer* b = new FBuffer(); b->fvf = fvf; b->count = count;
            const unsigned stride = fldraw::MakeLayout(fvf).stride;
            b->bytes.assign((size_t)stride * count, 0);
            vbs_.insert(b); *out = (unsigned*)b; return GR_OK;
        }
        GENRESULT DACOM_API DestroyVb(void** h) override { FBuffer* b = (FBuffer*)*h; if (vbs_.erase(b)) delete b; *h = nullptr; return GR_OK; }
        GENRESULT DACOM_API IsVbValid(void* h) override { return vbs_.count((FBuffer*)h) ? GR_OK : GR_GENERIC; }
        GENRESULT DACOM_API GetVbInfo(void* h, unsigned* fvf, unsigned* count) override { FBuffer* b = (FBuffer*)h; if (!vbs_.count(b)) return GR_INVALID_PARMS; *fvf = b->fvf; *count = b->count; return GR_OK; }
        GENRESULT DACOM_API LockVb(void* h, unsigned* offset, void** out, unsigned) override { FBuffer* b = (FBuffer*)h; if (!vbs_.count(b)) return GR_INVALID_PARMS; if (offset) *offset = 0; *out = b->bytes.data(); return GR_OK; }
        GENRESULT DACOM_API UnlockVb(void*) override { return GR_OK; }
        GENRESULT DACOM_API SelectVb(void* h) override
        {
            if (!h) { selVb_ = nullptr; selIb_ = nullptr; return (GENRESULT)0x80004005; }
            if (h == (void*)(intptr_t)-1) return selVb_ ? GR_OK : (GENRESULT)0x80004005;
            FBuffer* b = (FBuffer*)h;
            if (!vbs_.count(b)) return (GENRESULT)0x80004005;
            selVb_ = b; return GR_OK;
        }
        GENRESULT DACOM_API UnsupportedVertexBufferOperation(void*) override { return (GENRESULT)0xFFFFFFF9; }

        GENRESULT DACOM_API CreateIb(unsigned count, unsigned** out, unsigned) override { FIndexBuf* b = new FIndexBuf(); b->count = count; b->idx.assign(count, 0); ibs_.insert(b); *out = (unsigned*)b; return GR_OK; }
        GENRESULT DACOM_API DestroyIb(void** h) override { FIndexBuf* b = (FIndexBuf*)*h; if (ibs_.erase(b)) delete b; *h = nullptr; return GR_OK; }
        GENRESULT DACOM_API IsIbValid(void* h) override { return ibs_.count((FIndexBuf*)h) ? GR_OK : GR_GENERIC; }
        GENRESULT DACOM_API GetIbCapacity(void* h, unsigned* out) override { FIndexBuf* b = (FIndexBuf*)h; if (!ibs_.count(b)) return GR_INVALID_PARMS; *out = b->count; return GR_OK; }
        GENRESULT DACOM_API LockIb(void* h, unsigned* offset, void** out, unsigned count) override
        {
            // Handle -1 addresses the device's inline scratch index buffer (RP8: this+0x140), allocated by bump cursor.
            FIndexBuf* b = h == (void*)(intptr_t)-1 ? ScratchIb() : (FIndexBuf*)h;
            if (!ibs_.count(b)) return GR_INVALID_PARMS;
            unsigned at = 0;
            if (b == scratchIb_) { at = scratchUsed_; scratchUsed_ += count; if (b->idx.size() < (size_t)at + count) b->idx.resize((size_t)at + count + 1024); }
            if (offset) *offset = at;
            *out = b->idx.data() + at;
            return GR_OK;
        }
        GENRESULT DACOM_API UnlockIb(void*) override { return GR_OK; }
        GENRESULT DACOM_API SelectIb(void* h, unsigned base, unsigned, unsigned) override
        {
            if (!h) { selIb_ = nullptr; selIbBase_ = 0; return (GENRESULT)0x80004005; }
            FIndexBuf* b = h == (void*)(intptr_t)-1 ? ScratchIb() : (FIndexBuf*)h;
            if (!ibs_.count(b)) return (GENRESULT)0x80004005;
            selIb_ = b; selIbBase_ = base; return GR_OK;
        }
    };

    GENRESULT FInner::QueryInterface(const char* iid, void** out) { return Owner ? Owner->QueryLocal(iid, out) : GR_INTERFACE_UNSUPPORTED; }
    uint32_t FInner::Release()
    {
        if (Refs > 0) --Refs;
        if (Refs == 0) { ++Refs; delete Owner; Owner = nullptr; return 0; }
        return Refs;
    }

    // Descriptor RP8 / the system container pass to CreateInstance (FRp8AggDesc, RP8.h).
    struct FPipelineAggDesc
    {
        uint32_t Size;
        const char* Name;
        IDAComponent* Outer;
        IDAComponent** InnerOut;
        const char* Extra;
    };

    class FSokolFactory : public IComponentFactory
    {
        uint32_t refs_ = 1;
    public:
        GENRESULT DACOM_API QueryInterface(const char* iid, void** out) override
        {
            if (!out) return GR_GENERIC;
            *out = nullptr;
            if (strcmp(iid, Iids::ComponentFactory) != 0) return GR_INTERFACE_UNSUPPORTED;
            *out = static_cast<IComponentFactory*>(this);
            AddRef();
            return GR_OK;
        }
        uint32_t DACOM_API AddRef() override { return ++refs_; }
        uint32_t DACOM_API Release() override { if (--refs_ == 0) { delete this; return 0; } return refs_; }
        GENRESULT DACOM_API CreateInstance(AGGDESC* desc, void** instance) override
        {
            FPipelineAggDesc* d = reinterpret_cast<FPipelineAggDesc*>(desc);
            *instance = nullptr;
            if (d->Size != 0x14 || strcmp(d->Name, kRenderPipelineComponentName) != 0) return GR_INTERFACE_UNSUPPORTED;
            FSokolDevice* dev = new FSokolDevice();
            dev->SetOuter(d->Outer);
            if (d->Outer) *d->InnerOut = dev->Inner();
            *instance = static_cast<IRenderPipeline*>(dev);
            return GR_OK;
        }
    };
}

namespace fl { namespace render
{
    bool RegisterBackend(ICOManager* dacom)
    {
        if (!dacom) return false;
#if defined(__APPLE__)
        if (!fl::text::GetRasterizer()) fl::text::SetRasterizer(FlCreateCoreTextRasterizer());
#endif
        FSokolFactory* f = new FSokolFactory();
        dacom->RegisterComponent(f, kRenderPipelineComponentName, (void*)(uintptr_t)0x40000000 /* RP8.h kRp8RegisterPriority */);
        f->Release();
        return true;
    }
}}
#endif // FL_HAVE_SOKOL

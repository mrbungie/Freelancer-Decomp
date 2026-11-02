# sokol render backend (skeleton)

Implements the DACOM component `"DirectX8"` (`kRp8ComponentName`) on top of sokol_gfx, so the
unchanged engine (`Shading`, `RendComp`, `flmaterials`, `alchemy`, `ximage`, client) draws without Direct3D.
Sokol's backends cover Metal (macOS), GL/GLES, D3D11, WebGPU; a console port plugs its own
`sg_environment`/swapchain here and nothing else changes.

## What to implement (src/include/render/IRenderPipeline.h)
| Interface | Retail reference | Notes |
|---|---|---|
| `IRenderPipeline` (+`IRenderPipelineEx`) | RP8.cpp `FRp8Pipeline`, slots in `render/slots.h` | state setters, `Startup`, `BeginScene/EndScene`, mode changes, caps queries |
| `IRPDraw` | RP8.cpp draw entry points | primitive submit; map FVF + render states onto one `sg_pipeline` per state key (cache them) |
| `IVertexBufferManager`, `IRPVertexBuffer`, `IRPIndexBuffer` | RP8.cpp VB/IB classes | `sg_buffer`; lock/unlock = staging copy + `sg_update_buffer` |
| `IRPTexture`, `ITextureLibraryA` | RP8.cpp texture classes | `sg_image`; the D3DFMT_* -> `sg_pixel_format` table lives in `pixel_format.cpp` |
| `IGammaControl` | | swapchain/post pass |
| `IMaterial` (render/IMaterial.h) | flmaterials | fixed-function stage state -> shader permutations (generate shaders with sokol-shdc) |

The fixed-function pipeline (texture stages, fog, lights, alpha/z states) must be emulated in shaders:
start from the state keys RP8 already tracks (grep `SetRenderState` / `SetTextureStageState` in RP8.cpp).

## Wiring
* `sokol_render.cpp` here compiles to a stub that fails registration until sokol headers are vendored
  (`third_party/sokol/`) and `FL_HAVE_SOKOL` is defined.
* Window: take `fl::render::GetNativeWindow()` (an `SDL_Window*`) and build the sokol environment/swapchain
  from it in `IRenderPipeline::Startup`.

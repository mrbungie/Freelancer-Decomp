# Porting Freelancer-Decomp to macOS / Portable Platforms

## 1. Executive Strategy & Architecture Decisions

This document outlines the architectural decisions and execution plan for porting **Freelancer-Decomp** to modern non-Windows platforms (primarily macOS on Apple Silicon / x86_64, and Linux) without requiring Wine at runtime.

### High-Level Architecture Overview

```
+-------------------------------------------------------------------------+
|                           Freelancer Client                             |
|                        (freelancer / Common)                            |
+-------------------+--------------------+--------------------------------+
|    Window & Input |       Audio        |            Graphics            |
|       (SDL2)      |  (OpenAL Soft /    |  D3D8 / D3D9 (RP8 / RP9)       |
|                   |    miniaudio)      |              |                 |
|                   |                    |              v                 |
|                   |                    |         DXVK-Native            |
|                   |                    |              |                 |
|                   |                    |              v                 |
|                   |                    |        Vulkan API              |
|                   |                    |              |                 |
|                   |                    |              v (macOS)         |
|                   |                    |           MoltenVK             |
|                   |                    |              |                 |
|                   |                    |              v                 |
|                   |                    |            Metal               |
+-------------------+--------------------+--------------------------------+
|          Platform Abstraction & Synchronization Layer                   |
| (std::recursive_mutex, POSIX threads, portable C++ math, 64-bit shims) |
+-------------------------------------------------------------------------+
```

---

## 2. Key Architectural Decisions

### 2.1 Build System: Automated CMake Generation
- **Decision**: Replace Visual Studio MSBuild (`.sln` / 38 `.vcxproj` files) with modern CMake.
- **Mechanism**: A Python generator script (`tools/port/generate_cmake.py`) parses the project files, including source lists, project references, include directories, and preprocessor definitions, outputting a maintainable root `CMakeLists.txt`.
- **Rationale**: Eliminates manually maintaining 38 build files while allowing native compilation with Clang/Ninja on macOS.

### 2.2 Synchronization & Win32 Shims: Exact Semantics
- **Decision**: Win32 `CRITICAL_SECTION` is mapped strictly to `std::recursive_mutex` (never `std::mutex`).
- **Rationale**: Win32 critical sections are inherently recursive (a thread that already owns the critical section can enter it again without deadlocking). Using a non-recursive mutex would cause immediate deadlocks in recursive game state calls or message dispatch loops.
- **Location**: `src/include/platform/win_compat.h` and `src/include/platform/windows.h`.

### 2.3 Assembly (`__asm`) Replacement: Verified Equivalents Only
- **Decision**: **No blind regex transformations**. Every one of the 237 inline `__asm` blocks across 28 files is audited and replaced with verified C++ equivalents guarded by `#if defined(_M_IX86) && defined(_MSC_VER) ... #else ... #endif`.
- **Findings**:
  - `x87_trig.h`: 10 trigonometry functions (`X87Sin`, `X87Cos`, etc.) are exact mappings to standard C++ `<cmath>` functions (`std::sin`, `std::cos`, etc.).
  - `x86math.cpp`: Vector-matrix multiplication (3x4 affine transforms) and dot products (`FFastDot`). The decompilation authors already included pure C++ implementations under `#else` for `FFastDot`. Matrix operations are standard linear algebra with specific handling of aliased outputs.
  - `Thorn` / `RemoteCommon`: Majority of inline assembly was used to replicate VC6 FPU rounding / float assignments (`fld`/`fstp`). These map cleanly to standard C++ float operations.

### 2.4 Graphics Pipeline: Translation Over Complete Rewrite
- **Decision**: Preserve `RP8` (Direct3D 8) and `RP9` (Direct3D 9) by leveraging the translation chain:
  $$\text{Direct3D 8/9} \longrightarrow \text{DXVK-Native} \longrightarrow \text{Vulkan} \longrightarrow \text{MoltenVK} \longrightarrow \text{Metal}$$
- **Rationale**: Rewriting the entire fixed-function and programmable render pipeline from scratch in Apple Metal would require months of reverse engineering texture combiners, render states, and shaders. Using **DXVK-Native** (DXVK compiled as a native shared library rather than a Windows DLL) paired with **MoltenVK** provides a proven, high-performance path on macOS.

### 2.5 Windowing and Input
- **Decision**: Abstract the Win32 window loop (`WinMain`, `CreateWindowEx`, `PeekMessage`, `DispatchMessage`) and DirectInput with **SDL2**.
- **Rationale**: SDL2 provides rock-solid cross-platform window management, retina display support, and keyboard/mouse event handling on macOS.

### 2.6 Math Performance & Modern CPU Architecture
- **Current Portable Phase**: Standard C++ `<cmath>` (`std::sin`, `std::cos`, etc.) and direct matrix/quaternion arithmetic under `#else` in `x86math.cpp` and `x87_trig.h`.
- **Performance Characteristics**:
  - Original x87 `__asm` used the 1980 8087 8-register FPU stack (`fsin`/`fcos`). On modern x86_64 and ARM64 CPUs, x87 instructions are legacy microcode operations taking ~80-140 CPU cycles and blocking pipeline execution.
  - Standard C++ with Clang `-O2`/`-O3` compiles to modern scalar/vector instructions with Minimax polynomial approximations (~10-15 cycles), yielding a ~10x performance gain over the original 2003 assembly.
- **Future Optimization Path (Post-Bootstrap Backlog)**:
  - **Option 1 (Apple Accelerate / vDSP)**: Use macOS native `Accelerate.framework` vector transcendental functions (`vsinf`, `vcosf`) and AMX hardware matrix extensions.
  - **Option 2 (DirectXMath with NEON)**: Use open-source cross-platform `DirectXMath` for 128-bit SIMD FMA (Fused Multiply-Add) vector and matrix math.
  - Because DACOM isolates all 3D math behind `I3DMathEngine` in `x86math`, these optimizations can be dropped into `x86math` without touching any game logic.

### 2.7 Strategic Graphics Backend Architecture (IRenderPipeline)
In Freelancer, the graphics API is **completely decoupled** from the rest of the game:
- The entire engine (`Shading`, `RendComp`, `flmaterials`, `alchemy`, `ximage`, `Client`) communicates with the GPU strictly through DACOM interfaces defined in `src/include/render/IRenderPipeline.h` (`IRenderPipeline8B`, `IVertexBufferManager`, `IRPDraw`, `IRPVertexBuffer`, `IRPIndexBuffer`, `IRPTexture`).
- No game logic or asset loading code makes direct Direct3D calls.
- In `bin/DACOM.INI`, the client simply requests component `DirectX8` (originally loaded from `RP8.dll`, with `RP9.dll` existing as an in-progress D3D9 drop-in).

**Evaluation of Approaches**:
1. **Translation Chain (D3D8 -> DXVK-Native -> Vulkan -> MoltenVK -> Metal)**:
   - *Pros*: Reuses the existing `RP8`/`RP9` C++ reverse-engineered logic.
   - *Cons*: Heavy, multi-tiered translation pipeline. Requires Vulkan loader, MoltenVK runtime, shader SPIR-V transpilation, and DXVK-Native builds for macOS ARM64. Debugging graphics glitches across 4 translation layers is notoriously difficult.
2. **Native Modern Renderer (`RPModern` / `RPSDL` using Sokol-GFX, BGFX, or direct Metal)**:
   - *Pros*:
     - Porting once to a clean modern graphics abstraction directly implements the 7 interfaces of `IRenderPipeline`.
     - Freelancer is a 2003 game: its rendering model consists of basic vertex/index buffers, fixed-function or simple vertex shaders, 2D/cubemap textures, and standard alpha/depth states.
     - Direct Metal backend allows native Xcode Metal Frame Debugger, zero translation overhead, flawless Apple Silicon support, and clean portability to Linux (Vulkan/OpenGL) and Windows.
     - No external Wine or DXVK baggage.
   - *Decision Strategy*: Use minimal D3D8/D3D9 header definitions to compile all game/material logic cleanly, then implement a clean native rendering backend (`RPModern`) or lightweight translation layer directly behind `IRenderPipeline`.

### 2.8 Modular Verification Order (Layer by Layer)
Instead of attempting a monolithic build that compiles but crashes at runtime, modules are ported and verified bottom-up:
1. **Layer 0 (Math & Core Libraries)**: `zlib`, `qhull`, `x86math`.
2. **Layer 1 (Digital Anvil Engine Base)**: `dacom` (DACOM COM runtime), `ReadFile` (VFS), `EngBase`, `DALib`.
3. **Layer 2 (Graphics Shading & Intermediate Mesh)**: `Shading`, `flmaterials`, `RendComp`, `ximage`, `Deformable2`, `alchemy`.
4. **Layer 3 (Graphics & Audio Drivers)**: `RPModern` / `RP8` (Native or DXVK-Native), `SoundManager` (via OpenAL Soft / SDL_Audio).
5. **Layer 4 (Game Logic & Scripting)**: `Common`, `Content`, `Thorn`.
6. **Layer 5 (Networking & Client Executable)**: `rpclocal`, `Server`, `RemoteCommon`, `RemoteClient`, `RemoteServer`, `freelancer`.

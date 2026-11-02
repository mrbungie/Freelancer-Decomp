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

### 2.6 Modular Verification Order (Layer by Layer)
Instead of attempting a monolithic build that compiles but crashes at runtime, modules are ported and verified bottom-up:
1. **Layer 0 (Math & Core Libraries)**: `zlib`, `qhull`, `x86math`.
2. **Layer 1 (Digital Anvil Engine Base)**: `dacom` (DACOM COM runtime), `ReadFile` (VFS), `EngBase`, `DALib`.
3. **Layer 2 (Graphics Shading & Intermediate Mesh)**: `Shading`, `flmaterials`, `RendComp`, `ximage`, `Deformable2`, `alchemy`.
4. **Layer 3 (Graphics & Audio Drivers)**: `RP9` / `RP8` (via DXVK-Native), `SoundManager` (via OpenAL Soft / SDL_Audio).
5. **Layer 4 (Game Logic & Scripting)**: `Common`, `Content`, `Thorn`.
6. **Layer 5 (Networking & Client Executable)**: `rpclocal`, `Server`, `RemoteCommon`, `RemoteClient`, `RemoteServer`, `freelancer`.

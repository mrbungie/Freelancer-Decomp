
## Platform abstraction layer (src/platform)

OS services the game needs (files/find, well-known folders, clock, uuid/user name, registry
replacement) are interfaces in `src/platform/include/fl/platform.h`. Win32-named wrappers in
`src/include/platform/win_compat.h` forward to them. A backend lives in
`src/platform/backends/<name>/` and is chosen with `-DFL_PLATFORM_BACKEND=<name>` (default `posix`,
covers macOS and Linux). A new OS = a new backend directory.

Not yet behind an interface (compile-only stubs that call `fl::ReportUnimplemented`):
- GDI text rasteriser used by `Common/common_textrender.cpp` -> needs `ITextRasterizer`
  (CoreText / FreeType backends).
- MSXML SAX reader used by `Common/common_rdlio.cpp` -> `CoCreateInstance` fails; needs an
  `IXmlSax` backend (libxml2/expat).
- ACM (MP3/ADPCM) in SoundStreamer -> `platform/msacm.h` reports "no driver"; needs a software decoder.

Game-logic layer (Common, Content, Server, RemoteClient, RemoteServer, rpclocal, freelancer) is built
with `platform/lp64_layout.h` force-included: all `static_assert`s are compiled out because they pin
retail's 32-bit struct layouts. Layouts are NOT retail-compatible. Known blocker: the RPC codecs
(`rpc_packet_codec.h`, `Remote*`) keep marshal pointers in `unsigned int` and use x86 inline asm.

## Status: whole tree builds and links (arm64)

`cmake -S . -B build -G Ninja && ninja -C build` produces every library plus `build/freelancer`.
Not built by default (`EXCLUDED_TARGETS` in tools/port/generate_cmake.py): `movie` (the client
loads movie.dll dynamically and skips movies when absent), `RP8`/`RP9` (Direct3D, to be replaced by RPModern).

The executable links but **segfaults at startup** (not yet diagnosed); it cannot run until these exist:
1. Window creation: `CreateWindowEx`/`RegisterClass`/`DialogBoxParam` are reporting stubs; the SDL
   front end (`src/platform/frontend/sdl/sdl_main.cpp`) must create the window and feed WinMain's message loop.
2. Render backend: an `IRenderPipeline` implementation (RPModern on Metal/sokol/bgfx).
3. Input: DirectInput is stubbed (`DirectInput8Create` fails) -> SDL-backed devices.
4. Text rasteriser (GDI), XML (MSXML SAX for RDL), MP3/ADPCM (ACM), IME bridge, registry (file-backed).

Compile-only stubs always call `fl::ReportUnimplemented(...)` so each gap shows up once on stderr.
Shim headers live in `src/include/platform/` (`win_compat.h`, `win_ui_compat.h`, `imm.h`, `msacm.h`, ...).

## Extension points (multiplatform / Xbox 360 / sokol)

| Concern | Where | Selected by |
|---|---|---|
| OS services (files, folders, clock, uuid, registry, window/cursor host) | `src/platform/include/fl/platform.h`, backends in `src/platform/backends/<name>/` | `-DFL_PLATFORM_BACKEND=posix` (default) |
| SDL generation | `src/platform/frontend/sdl/fl_sdl.{h,cpp}` — the only file that includes SDL. `FL_SDL_HAS_*` capability macros | `-DFL_SDL_MAJOR=2` (default) or `1` (SDL 1.2 branch written, **not compiled yet**) |
| GPU / render pipeline | `src/platform/include/fl/render_backend.h`; implement `fl::render::RegisterBackend` in `src/platform/render/<name>/` to register the DACOM component `"DirectX8"` | `-DFL_RENDER_BACKEND=null` (default) or `sokol` (skeleton + README with the interface table) |
| PE resources (strings, infocards) | `src/platform/include/fl/pe_module.h` — reads retail `Resources.dll`, `InfoCards.dll`, … on any host. Search order: cwd, `$FL_RESOURCE_DIR`, `../Cab1/exe`, `Cab1/exe` | automatic |
| Win32 path / case differences | `fl::IFileSystem::ResolvePath`, used by every file wrapper in `win_compat.h` | automatic |

## Running (current state)

`bin/freelancer` is built in place next to the retail `.ini` files (`DATA/` is `..\DATA`). Run it from `bin/`.
`FL_TRACE_MESSAGEBOX=1` prints a backtrace whenever the game raises a message box.
A SIGSEGV/SIGBUS/SIGABRT handler prints a backtrace (`sdl_main.cpp`).

Verified progress: static init, WinMain, ini/INI_Reader, resource DLL loading, DACOM initialisation.
Currently stops in `input_manager.cpp` `CreateDevice` (null DirectInput device).

## Module architecture (decision pending)

Retail is ~30 DLLs; DACOM `LoadLibrary`s each one and its `DllMain` registers COM factories. Linking them all
statically collides (each DLL had its own `GetClientInterface`, `FDacomFactory`, `Client`, `Server`, ...).
Faithful fix: build each former DLL as a **shared library** (`.dylib`/`.so`) and map `LoadLibrary("X.dll")` to
`dlopen("libX.dylib")` + call its `DllMain` — `LoadLibraryA` in `win_compat.h` already tries the in-exe table
(`builtin_modules_generated.cpp`), then PE resources, then `dlopen`. Until then only DACOM-core modules are linked
in, so the `[Libraries]` list in `DACOM.INI` finds nothing to register.

## Known hazard of the 64-bit layout build

`lp64_layout.h` disables every `static_assert`. Code that type-puns between structs which are laid out
identically only with 4-byte pointers/`long` breaks silently (found: `FPanelVertexBatcher` vs
`FEffectVertexBatch`, fixed by changing `unsigned long` -> `unsigned`). Audit plan: build once with
`-DFL_STRICT_LAYOUT` and triage the failures that are *not* explained by pointer width.

## LP64 audit tooling and findings (2026-11-02)

`python3 tools/port/lp64_audit.py` (≈2 min) does the sweep instead of run-crash-fix cycles:
(1) builds with `-DFL_STRICT_LAYOUT=ON` (layout static_asserts on) and groups failures per file (most are plain
pointer growth, not bugs); (2) scans for the real silent hazards: facet/offset arithmetic, `{IID, offset}` tables,
`*FromNested/NestedThis`, descriptor sizes via `sizeof`, pointer<->32-bit casts.

Classes of LP64 bug found so far (all fixed unless noted):
- `GENRESULT` was `long`: `(GENRESULT)0xFFFFFFFD` is positive on LP64, so every `>= 0` success test passed on errors. Now `int32_t`.
- Descriptor `Size` fields are a wire protocol (`0x14`, `0x8`, ...), not `sizeof()` of the wider C++ struct.
- `{IID, byte-offset}` QueryInterface tables and `this +/- N` facet recovery (EngBase, Deformable2, system, Thorn
  component, droppers, guide behavior, TextureLibrary): replaced by typed `static_cast`s.
- Same-layout type punning (`FPanelVertexBatcher` vs `FEffectVertexBatch`).
- NOT fixed: `flmaterials/MaterialImpl.cpp` treats parameter tables as raw arrays of 32-bit words holding pointers
  (`SrcWords[1]`, `Entry+4`); needs the table structs rewritten with real pointer members. Likely similar in other
  data-driven tables; the pattern scan does not see them (they index `unsigned long*`).
- Dead code: `src/include/server_cobject.h` / `server_cship.h` (hand-made FCObject view with retail offsets) are unused.

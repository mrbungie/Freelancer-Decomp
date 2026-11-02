#!/usr/bin/env python3
"""
generate_cmake.py
Parses Visual Studio solution and .vcxproj files to generate a modern CMake build system
for Freelancer-Decomp targeting macOS and other non-Windows platforms.
"""

import os
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
SRC_DIR = REPO_ROOT / "src"

# Projects to skip (Windows-only UI / tooling / pure resource DLLs)
EXCLUDED_PROJECTS = {
    "FLServer",          # Dedicated server MFC UI
    "MemoryDiagnostics", # Internal leak tracker
    "resources",         # Pure resource DLL
    "InfoCards",         # Pure resource DLL
    "MiscText",          # Pure resource DLL
    "NameResources",     # Pure resource DLL
    "EquipResources",    # Pure resource DLL
    "OfferBribeResources", # Pure resource DLL
    "MiscTextInfo2",     # Pure resource DLL
    "serverresources",   # Pure resource DLL
}

# Namespace map for MSBuild XML
NS = {"ms": "http://schemas.microsoft.com/developer/msbuild/2003"}

# movie: DirectShow/WMF player, loaded by the client via LoadLibrary("movie.dll") and skipped when
# absent (engine_init.cpp PlayStartupMovies). Needs an IVideoPlayer backend before it is useful.
# RP8/RP9: Direct3D 8/9 render pipelines; replaced by the RPModern backend (see PORTING_MACOS.md).
EXCLUDED_TARGETS = {"movie", "RP8", "RP9"}
ALL_LIBS = []
ALL_LIBS_FOR_EXE = []
LP64_LAYOUT_TARGETS = {"Common", "Content", "Server", "RemoteClient", "RemoteServer", "rpclocal", "freelancer"}


def parse_vcxproj(vcxproj_path: Path):
    if not vcxproj_path.exists():
        return None
    
    try:
        tree = ET.parse(vcxproj_path)
        root = tree.getroot()
    except Exception as e:
        print(f"Error parsing {vcxproj_path}: {e}")
        return None

    project_dir = vcxproj_path.parent
    project_name = vcxproj_path.stem

    # Detect configuration type (DynamicLibrary, StaticLibrary, Application)
    config_type = "DynamicLibrary"
    for prop in root.findall(".//ms:ConfigurationType", NS):
        if prop.text:
            config_type = prop.text.strip()
            break
    
    # Collect source files (.cpp, .c)
    sources = []
    for item in root.findall(".//ms:ClCompile", NS):
        src_file = item.get("Include")
        if src_file and not src_file.endswith(".h"):
            # Resolve relative to project_dir and normalize posix slashes
            normalized = (project_dir / src_file).resolve()
            rel_to_src = os.path.relpath(normalized, SRC_DIR).replace("\\", "/")
            # Avoid duplicate files
            if rel_to_src not in sources:
                sources.append(rel_to_src)

    # Collect include files (.h, .hpp)
    headers = []
    for item in root.findall(".//ms:ClInclude", NS):
        hdr_file = item.get("Include")
        if hdr_file:
            normalized = (project_dir / hdr_file).resolve()
            rel_to_src = os.path.relpath(normalized, SRC_DIR).replace("\\", "/")
            if rel_to_src not in headers:
                headers.append(rel_to_src)

    # Collect project dependencies
    dependencies = []
    for item in root.findall(".//ms:ProjectReference", NS):
        dep_path = item.get("Include")
        if dep_path:
            dep_name = Path(dep_path.replace("\\", "/")).stem
            if dep_name not in EXCLUDED_PROJECTS:
                dependencies.append(dep_name)

    # Check for additional includes
    add_includes = []
    for item in root.findall(".//ms:AdditionalIncludeDirectories", NS):
        if item.text:
            for inc in item.text.split(";"):
                inc = inc.strip()
                if inc and "%" not in inc:
                    inc_path = inc.replace("$(ProjectDir)", "").replace("$(MSBuildThisFileDirectory)", "")
                    inc_path = inc_path.replace("\\", "/")
                    if inc_path:
                        add_includes.append(inc_path)

    # Preprocessor definitions
    defines = []
    for item in root.findall(".//ms:PreprocessorDefinitions", NS):
        if item.text:
            for d in item.text.split(";"):
                d = d.strip()
                if d and "%" not in d and d not in ("WIN32", "_CRT_SECURE_NO_WARNINGS"):
                    defines.append(d)

    return {
        "name": project_name,
        "dir": os.path.relpath(project_dir, SRC_DIR),
        "config_type": config_type,
        "sources": sorted(sources),
        "headers": sorted(headers),
        "dependencies": sorted(set(dependencies)),
        "add_includes": sorted(set(add_includes)),
        "defines": sorted(set(defines)),
    }

def main():
    print(f"Scanning solutions and projects in {SRC_DIR}...")
    projects = []
    for vcxproj in sorted(SRC_DIR.glob("**/*.vcxproj")):
        name = vcxproj.stem
        if name in EXCLUDED_PROJECTS:
            continue
        p_info = parse_vcxproj(vcxproj)
        if p_info and p_info["sources"]:
            projects.append(p_info)
            print(f"  Found project: {name} ({p_info['config_type']}, {len(p_info['sources'])} sources)")

    # Build root CMakeLists.txt
    cmake_lines = [
        "# Auto-generated CMakeLists.txt for Freelancer-Decomp (macOS / Portable)",
        "# Generated by tools/port/generate_cmake.py",
        "cmake_minimum_required(VERSION 3.20)",
        "project(Freelancer C CXX)",
        "if(APPLE)",
        "    enable_language(OBJCXX)",
        "endif()",
        "",
        "set(CMAKE_CXX_STANDARD 17)",
        "set(CMAKE_CXX_STANDARD_REQUIRED ON)",
        "set(CMAKE_C_STANDARD 11)",
        "set(CMAKE_POSITION_INDEPENDENT_CODE ON)",
        "",
        "# Global includes and compatibility shims",
        "include_directories(",
        "    ${CMAKE_CURRENT_SOURCE_DIR}/src/include",
        "    ${CMAKE_CURRENT_SOURCE_DIR}/src/include/platform",
        "    ${CMAKE_CURRENT_SOURCE_DIR}/src/include/platform/dx",
        ")",
        "",
        "if(APPLE)",
        "    add_compile_definitions(PLATFORM_MACOS=1 __APPLE__=1)",
        "    find_package(PkgConfig REQUIRED)",
        "    add_compile_options(",
        "        -g",
        "        -Wno-unknown-pragmas",
        "        -Wno-macro-redefined",
        "        -Wno-deprecated-declarations",
        "        -Wno-writable-strings",
        "        -Werror=pointer-to-int-cast",
        "        -Werror=void-pointer-to-int-cast",
        "        -Werror=int-to-pointer-cast",
        "        -Werror=int-to-void-pointer-cast",
        "        -ferror-limit=0",
        "        -Wno-invalid-offsetof",
        "        -fdeclspec",
        "        -fms-extensions",
        "    )",
        "endif()",
        "",
        "# Common compile definitions for portability",
        "# Retail shipped ~30 DLLs with colliding symbols; keep that shape: former DLLs are shared libraries",
        "# loaded with dlopen by LoadLibraryA (see PORTING_MACOS.md, Module architecture).",
        "option(FL_MODULES_SHARED \"Build former DLL modules as shared libraries\" ON)",
        "if(FL_MODULES_SHARED)",
        "    set(FL_DLL_TYPE SHARED)",
        "    set(CMAKE_POSITION_INDEPENDENT_CODE ON)",
        "else()",
        "    set(FL_DLL_TYPE STATIC)",
        "endif()",
        "",
        "# Platform abstraction layer (src/platform): pick a backend with -DFL_PLATFORM_BACKEND=<dir>",
        "set(FL_PLATFORM_BACKEND \"posix\" CACHE STRING \"Platform backend under src/platform/backends\")",
        "file(GLOB FL_PLATFORM_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/backends/${FL_PLATFORM_BACKEND}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/common/*.cpp)",
        "add_library(flplatform ${FL_DLL_TYPE} ${FL_PLATFORM_SOURCES})",
        "if(FL_MODULES_SHARED)",
        "    set_target_properties(flplatform PROPERTIES PREFIX \"\" LIBRARY_OUTPUT_DIRECTORY ${FL_OUTPUT_DIR})",
        "endif()",
        "",
        "# Output directory for the executable and the module .dylibs (retail layout: next to the .ini files).",
        "set(FL_OUTPUT_DIR \"${CMAKE_CURRENT_SOURCE_DIR}/bin\" CACHE PATH \"Where freelancer and its modules are built\")",
        "option(FL_ASAN \"Build everything with AddressSanitizer (memory-bug hunting; use with FL_OUTPUT_DIR=<repo>/bin-asan)\" OFF)",
        "if(FL_ASAN)",
        "    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)",
        "    add_link_options(-fsanitize=address)",
        "endif()",
        "",
        "# -DFL_STRICT_LAYOUT=ON re-enables retail's static_asserts (see tools/port/lp64_audit.py)",
        "option(FL_STRICT_LAYOUT \"Keep layout static_asserts (expected to fail on 64-bit; used by the audit)\" OFF)",
        "if(FL_STRICT_LAYOUT)",
        "    add_compile_definitions(FL_STRICT_LAYOUT=1)",
        "    add_compile_options(-ferror-limit=0)",
        "endif()",
        "",
        "# reinterpret_cast of a <64-bit integer to a pointer goes through fl::HandleToPtr (platform/handle_casts.h)",
        "add_compile_options(\"SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/src/include/platform/handle_casts.h\")",
        "",
        "# Render backend (src/platform/render/<name>, see fl/render_backend.h): null | sokol | <your own>",
        "set(FL_RENDER_BACKEND \"sokol\" CACHE STRING \"Render backend under src/platform/render\")",
        "file(GLOB FL_RENDER_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/render/${FL_RENDER_BACKEND}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/render/${FL_RENDER_BACKEND}/*.mm)",
        "add_library(flrender STATIC ${FL_RENDER_SOURCES})",
        "target_include_directories(flrender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/include ${CMAKE_CURRENT_SOURCE_DIR}/src/include/platform)",
        "if(FL_RENDER_BACKEND STREQUAL \"sokol\" AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sokol/sokol_gfx.h)",
        "    target_compile_definitions(flrender PRIVATE FL_HAVE_SOKOL=1)",
        "    target_include_directories(flrender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sokol)",
        "    if(APPLE)",
        "        target_compile_options(flrender PRIVATE $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)",
        "        target_link_libraries(flrender PUBLIC \"-Wl,-framework,Metal\" \"-Wl,-framework,QuartzCore\" \"-Wl,-framework,Foundation\" \"-Wl,-framework,CoreText\" \"-Wl,-framework,CoreGraphics\")",
        "    endif()",
        "endif()",
        "",
        "# SDL generation for the front end: 2 (default) or 1 (e.g. console ports shipping SDL 1.2)",
        "set(FL_SDL_MAJOR 2 CACHE STRING \"SDL major version used by src/platform/frontend/sdl\")",
        "add_compile_definitions(FL_SDL_VERSION=${FL_SDL_MAJOR})",
        "find_package(PkgConfig REQUIRED)",
        "if(FL_SDL_MAJOR EQUAL 1)",
        "    pkg_check_modules(FLSDLPKG REQUIRED sdl)",
        "else()",
        "    pkg_check_modules(FLSDLPKG REQUIRED sdl2)",
        "endif()",
        "include_directories(${FLSDLPKG_INCLUDE_DIRS})",
        "link_directories(${FLSDLPKG_LIBRARY_DIRS})",
        "target_include_directories(flplatform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/include)",
        "include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/platform/include)",
        "",
        "add_compile_definitions(",
        "    _CRT_SECURE_NO_WARNINGS=1",
        "    _CRT_NONSTDC_NO_DEPRECATE=1",
        "    FREELANCER_PORTABLE=1",
        ")",
        "",
    ]

    # Build targets
    import re as _re
    def _has_dllmain(proj):
        for src in proj["sources"]:
            f = REPO_ROOT / "src" / src
            try:
                if _re.search(r"\bDllMain\s*\(", f.read_text(errors="ignore")):
                    return True
            except OSError:
                pass
        return False
    libproj = [p for p in projects if p["config_type"] != "Application" and p["name"] not in EXCLUDED_TARGETS]
    exe_deps = set()
    for _p in projects:
        if _p["config_type"] == "Application":
            exe_deps.update(_p["dependencies"])
    # Only modules actually linked into the executable can be listed (see PORTING_MACOS.md,
    # "Module architecture": the rest need the shared-library build).
    libs = sorted(p["name"] for p in libproj if _has_dllmain(p) and p["name"] in exe_deps)
    ALL_LIBS_FOR_EXE.extend(sorted(p["name"] for p in libproj))
    for p in projects:
        name = p["name"]
        target_type = "add_library"
        is_exe = (p["config_type"] == "Application")
        if is_exe:
            target_type = "add_executable"

        cmake_lines.append(f"# Target: {name}")
        excl = " EXCLUDE_FROM_ALL" if name in EXCLUDED_TARGETS else ""
        if excl:
            cmake_lines.append("# Not built by default: see EXCLUDED_TARGETS in tools/port/generate_cmake.py")
        if is_exe or p["config_type"] != "DynamicLibrary":
            cmake_lines.append(f"{target_type}({name}{excl}")
        else:
            cmake_lines.append(f"{target_type}({name} ${{FL_DLL_TYPE}}{excl}")
        for s in p["sources"]:
            cmake_lines.append(f"    ${{CMAKE_CURRENT_SOURCE_DIR}}/src/{s}")
        if is_exe:
            cmake_lines.append("    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/frontend/sdl/sdl_main.cpp")
            cmake_lines.append("    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/frontend/sdl/fl_sdl.cpp")
            cmake_lines.append("    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/frontend/sdl/sdl_mouse.cpp")
            cmake_lines.append("    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/frontend/sdl/implicit_modules_generated.cpp")
        cmake_lines.append(")")
        cmake_lines.append("")

        # Target includes
        proj_dir_rel = p["dir"]
        cmake_lines.append(f"target_include_directories({name} PRIVATE")
        cmake_lines.append(f"    ${{CMAKE_CURRENT_SOURCE_DIR}}/src/{proj_dir_rel}")
        for inc in p["add_includes"]:
            cmake_lines.append(f"    ${{CMAKE_CURRENT_SOURCE_DIR}}/src/{proj_dir_rel}/{inc}")
        cmake_lines.append(")")

        # Target definitions
        if p["defines"]:
            defs_str = " ".join(f"-D{d}" for d in p["defines"])
            cmake_lines.append(f"target_compile_definitions({name} PRIVATE {defs_str})")

        # Retail built each module as a DLL; statically linked, per-module entry points and
        # "module's own copy" helpers would collide, so give them module-unique names.
        if not is_exe:
            cmake_lines.append(f"if(NOT FL_MODULES_SHARED)\n    target_compile_definitions({name} PRIVATE DllMain=DllMain_{name})\nendif()")
            if p["config_type"] == "DynamicLibrary":
                cmake_lines.append(
                    f"if(FL_MODULES_SHARED)\n"
                    f"    set_target_properties({name} PROPERTIES PREFIX \"\" LIBRARY_OUTPUT_DIRECTORY ${{FL_OUTPUT_DIR}})\n"
                    f"    target_link_options({name} PRIVATE -Wl,-undefined,dynamic_lookup)\n"
                    f"endif()")
        else:
            cmake_lines.append(f"target_compile_definitions({name} PRIVATE Acquire3DMathEngine=Acquire3DMathEngine_{name})")

        if is_exe:
            # Retail layout: the exe lives in <install>/EXE (here: bin/) next to its .ini files, with
            # ..\DATA beside it. Build straight into bin/ so it runs in place.
            cmake_lines.append(f"set_target_properties({name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${{FL_OUTPUT_DIR}} BUILD_RPATH @executable_path)")

        # Game-logic layer: layout static_asserts pin retail's 32-bit ABI (see lp64_layout.h)
        if name in LP64_LAYOUT_TARGETS:
            cmake_lines.append(
                f"target_compile_options({name} PRIVATE -include "
                f"${{CMAKE_CURRENT_SOURCE_DIR}}/src/include/platform/lp64_layout.h)")

        # Target dependencies
        _apps = {q["name"] for q in projects if q["config_type"] == "Application"}
        deps = [d for d in p["dependencies"] if d not in _apps or is_exe] + ["flplatform"] + (["flrender"] if is_exe else [])
        if is_exe and "APPLE" in ("APPLE"):
            deps.append("${FLSDLPKG_LIBRARIES}")
            deps.append("-Wl,-framework,Cocoa")
            deps.append("-Wl,-framework,IOKit")
            deps.append("-Wl,-framework,CoreVideo")

        if deps:
            cmake_lines.append(f"target_link_libraries({name} PRIVATE")
            for d in deps:
                cmake_lines.append(f"    {d}")
            cmake_lines.append(")")

        cmake_lines.append("")

    # Modules the executable links against directly are loaded by dyld, not by LoadLibrary, so their
    # DllMain never runs; main() "LoadLibrary"s these names once to run it (DLL_PROCESS_ATTACH).
    exe_deps = []
    for _p in projects:
        if _p["config_type"] == "Application" and _p["name"] == "freelancer":
            exe_deps = [d for d in _p["dependencies"] if d in {q["name"] for q in libproj}]
    gen = ["// GENERATED by tools/port/generate_cmake.py -- do not edit.",
           "// Modules linked directly into the executable; main() runs their DllMain via LoadLibraryA.",
           "extern const char* const FlImplicitModules[] = {"]
    gen += [f'    "{n}.dll",' for n in sorted(exe_deps)]
    gen += ["    nullptr", "};", ""]
    (REPO_ROOT / "src/platform/frontend/sdl/implicit_modules_generated.cpp").write_text("\n".join(gen), encoding="utf-8")

    out_cmake = REPO_ROOT / "CMakeLists.txt"
    out_cmake.write_text("\n".join(cmake_lines), encoding="utf-8")
    print(f"\nWrote root CMakeLists.txt to {out_cmake}")

if __name__ == "__main__":
    main()

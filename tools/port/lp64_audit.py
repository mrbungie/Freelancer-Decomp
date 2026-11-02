#!/usr/bin/env python3
"""LP64 audit: finds code that depends on retail's 32-bit object layout, in one pass (no running the game).

  1. Strict-layout build: compiles every module with the layout static_asserts ENABLED
     (-DFL_STRICT_LAYOUT=ON in a separate build dir) and groups the failures by source file.
     Each failure is a spot where the decompilation pinned a 32-bit size/offset.
  2. Pattern scan for code that computes addresses from retail offsets, which asserts do not catch:
       - this/object pointer +- constant (facet recovery, "FromNested", "NestedThis")
       - {IID, offset} tables
       - descriptor sizes compared against sizeof() of a struct that holds pointers/long
       - 'long' / 'unsigned long' members in structs that cross module boundaries

Usage:  python3 tools/port/lp64_audit.py [--no-build] > audit.txt
"""
import re, subprocess, sys, collections, pathlib

ROOT = pathlib.Path(__file__).resolve().parents[2]
BUILD = ROOT / "build-strict"

def build():
    BUILD.mkdir(exist_ok=True)
    subprocess.run(["cmake", "-S", str(ROOT), "-B", str(BUILD), "-G", "Ninja", "-DFL_STRICT_LAYOUT=ON",
                    "-DFL_MODULES_SHARED=OFF"], check=True, capture_output=True)
    r = subprocess.run(["ninja", "-C", str(BUILD), "-k", "0", "-j8"], capture_output=True, text=True)
    return r.stdout + r.stderr

def parse_asserts(log):
    found = collections.OrderedDict()
    for m in re.finditer(r"^(/[^:\n]+):(\d+):\d+: error: static assertion failed(?: due to requirement '([^']*)')?:? ?(.*)$", log, re.M):
        path, line, req, msg = m.groups()
        found[(path.replace(str(ROOT) + "/", ""), int(line))] = (req, msg.strip())
    return found

PATTERNS = [
    ("facet/offset arithmetic", r"\(\s*(?:unsigned\s+)?char\s*\*\s*\)\s*\w+\s*[-+]\s*(?:0x[0-9a-fA-F]+|\d+)\s*\)"),
    ("NestedThis/FromNested", r"\b\w*(?:NestedThis|FromNested|FromFacet)\w*\s*\("),
    ("iid/offset table", r"\{\s*(?:Iids::\w+|\"1\.11_\w+\")\s*,\s*(?:0x[0-9a-fA-F]+|\d+)\s*\}"),
    ("descriptor size via sizeof", r"(?:Size|size)\s*(?:!=|==|=)\s*sizeof\(\s*\w*(?:Desc|AGGDESC)\w*\s*\)"),
    ("pointer stored in 32-bit int", r"reinterpret_cast<\s*(?:unsigned\s+int|unsigned|int|DWORD|unsigned\s+long)\s*>\s*\("),
    ("C-cast pointer to 32-bit", r"\(\s*(?:unsigned\s+int|DWORD|int)\s*\)\s*(?:this|&\w+)\b"),
    # Found by hand while bringing the client up; each one was a silent heap overflow or a wrong-offset read.
    ("typed alloc with retail literal size", r"static_cast<[A-Z]\w*\*>\((?:::)?operator new\((?:0x[0-9a-fA-F]+|\d+)u?\b"),
    ("pointer array sized * 4", r"(?:new unsigned char|malloc|operator new)\(.*\*\s*4\)"),
    ("pointer from 32-bit int via uintptr_t", r"reinterpret_cast<[^>]*\*>\(static_cast<(?:uintptr_t|size_t|INT_PTR|intptr_t)>\("),
    ("record field read at retail dword offset", r"\(\((?:const )?(?:uint32_t|unsigned int|unsigned|DWORD)\*\)\w+\)\[\d+\]"),
    ("pointer read at literal Entry+N", r"\*\((?:const )?\w+\*(?: const)?\*\)\(\w+\s*\+\s*(?:0x[0-9a-fA-F]+|\d+)\)"),
    ("message params as uint32 array", r"Parms\[[1-9]\]"),
]

def scan():
    out = collections.defaultdict(list)
    for p in list((ROOT / "src").rglob("*.cpp")) + list((ROOT / "src").rglob("*.h")):
        rel = str(p.relative_to(ROOT))
        if "/IVP/" in rel or "/lua32/" in rel or rel.startswith("src/platform"):
            continue
        try:
            text = p.read_text(errors="ignore").splitlines()
        except OSError:
            continue
        for i, line in enumerate(text, 1):
            if line.lstrip().startswith("//"):
                continue
            for name, rx in PATTERNS:
                if re.search(rx, line):
                    out[name].append((rel, i, line.strip()[:110]))
    return out

def main():
    log = "" if "--no-build" in sys.argv else build()
    if log:
        (BUILD / "audit-build.log").write_text(log)
    elif (BUILD / "audit-build.log").exists():
        log = (BUILD / "audit-build.log").read_text()
    asserts = parse_asserts(log)
    byfile = collections.Counter(k[0] for k in asserts)
    print(f"== 1. static_assert failures (unique): {len(asserts)} in {len(byfile)} files")
    for f, n in byfile.most_common(60):
        print(f"{n:5d}  {f}")
    print()
    pats = scan()
    for name, hits in pats.items():
        print(f"== 2. {name}: {len(hits)}")
        for rel, ln, txt in hits[:80]:
            print(f"   {rel}:{ln}: {txt}")
        if len(hits) > 80:
            print(f"   ... {len(hits) - 80} more")
        print()

if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Mechanical LP64 fix: retail's `long`/`unsigned long` are 32-bit (MSVC x86). On LP64 they are 64-bit, which breaks
struct layouts, wire protocols and sign tests. Rewrites them to fixed-width types in the decompiled sources.
  unsigned long [int] -> uint32_t        long [int] / signed long -> int32_t
Leaves `long long`, `long double`, comments and string literals alone. Idempotent."""
import re, sys, pathlib
ROOT = pathlib.Path(__file__).resolve().parents[2] / "src"
SKIP_DIRS = ("IVP/", "lua32/", "platform/", "include/platform/", "luaprofile/lua32/", "zlib/vendor/", "qhull/", "include/platform/dx/")
def protect(text):
    # split into code / comment / string chunks
    out = []; i = 0; n = len(text)
    pat = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)
    last = 0
    for m in pat.finditer(text):
        out.append(("code", text[last:m.start()])); out.append(("lit", m.group(0))); last = m.end()
    out.append(("code", text[last:]))
    return out
R_UL = re.compile(r'\bunsigned\s+long\b(?!\s+long)(?:\s+int\b)?')
R_L = re.compile(r'(?<!unsigned )(?<!uint32_t )\b(?:signed\s+)?long\b(?!\s+long)(?!\s+double)(?:\s+int\b)?')
R_LL = re.compile(r'\blong\s+long\b')
def conv(code):
    code = R_LL.sub('@@LL@@', code)          # keep `long long` / `unsigned long long` / `long double` intact
    code = re.sub(r'\blong\s+double\b', '@@LD@@', code)
    code = R_UL.sub('uint32_t', code)
    code = R_L.sub('int32_t', code)
    return code.replace('@@LL@@', 'long long').replace('@@LD@@', 'long double')
changed = 0
for p in list(ROOT.rglob("*.cpp")) + list(ROOT.rglob("*.h")) + list(ROOT.rglob("*.inc")) + list(ROOT.rglob("*.c")):
    rel = str(p.relative_to(ROOT))
    if rel.startswith(SKIP_DIRS) or any(("/" + d) in "/" + rel for d in ("IVP/", "lua32/")):
        continue
    try: t = p.read_text(errors="surrogateescape")
    except OSError: continue
    if "long" not in t: continue
    nt = "".join(c if k == "lit" else conv(c) for k, c in protect(t))
    if nt != t:
        p.write_text(nt, errors="surrogateescape"); changed += 1
print("files changed:", changed)

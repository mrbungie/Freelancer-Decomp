#!/usr/bin/env python3
"""Rewrites 32-bit pointer<->integer casts flagged by -Werror=(void-)pointer-to-int-cast / int-to-(void-)pointer-cast.

  usage: fix_ptr_casts.py build.log
Two modes, chosen per source file:
  int   : values are small engine ids stored in pointers      -> (T)(intptr_t)(x)   and   (P*)(intptr_t)(int32_t)(x)
  table : values are real pointers squeezed into 32-bit words -> fl::PtrToHandle(x) and   (P*)fl::HandleToPtr(x)
"""
import re, sys, collections, pathlib
ROOT = pathlib.Path(__file__).resolve().parents[2]
INT_DIRS = ("src/Common/", "src/freelancer/Client/", "src/Deformable2/", "src/IVP/", "src/Thorn/lua32/", "src/luaprofile/lua32/")
INT_EXCEPT = ("swapped_material.cpp",)
def mode_for(rel):
    return "int" if rel.startswith(INT_DIRS) and not rel.endswith(INT_EXCEPT) else "table"

def match_close(s, i, open_c, close_c):
    depth = 0
    while i < len(s):
        c = s[i]
        if c == open_c: depth += 1
        elif c == close_c:
            depth -= 1
            if depth == 0: return i
        i += 1
    return -1

def match_open(s, j):
    depth = 0
    for k in range(j, -1, -1):
        if s[k] == ')': depth += 1
        elif s[k] == '(':
            depth -= 1
            if depth == 0: return k
    return -1

def operand_end(s, i):
    n = len(s)
    while i < n and s[i] in " \t": i += 1
    while i < n and s[i] in "&*!-~": i += 1
    while i < n and s[i] in " \t": i += 1
    if i < n and s[i] == '(':
        j = match_close(s, i, '(', ')')
        if j < 0: return -1
        i = j + 1
        k = i
        while k < n and s[k] in " \t": k += 1
        # `(T)operand`: a parenthesised type directly followed by an operand is a cast prefix -> keep going
        if k < n and (s[k].isalpha() or s[k] in "_(") and re.fullmatch(r'[\w:\s\*&<>]+', s[s.rfind('(', 0, j) + 1 if False else 0:0] or 'x') and re.fullmatch(r'\s*[A-Za-z_][\w:\s\*&]*', s[match_open(s, j) + 1:j]):
            return operand_end(s, k)
    else:
        m = re.compile(r'[A-Za-z_][\w:]*').match(s, i)
        if not m: return -1
        i = m.end()
        if s.startswith('<', i) and re.search(r'(?:static|reinterpret|const|dynamic)_cast$', s[:i]):
            j = match_close(s, i, '<', '>'); i = j + 1
    while i < n:
        if s[i] == '(':
            j = match_close(s, i, '(', ')');  i = j + 1
        elif s[i] == '[':
            j = match_close(s, i, '[', ']');  i = j + 1
        elif s.startswith('->', i):
            m = re.compile(r'->\s*[A-Za-z_]\w*').match(s, i)
            if not m: break
            i = m.end()
        elif s[i] == '.' and re.match(r'\.\s*[A-Za-z_]', s[i:i+3]):
            m = re.compile(r'\.\s*[A-Za-z_]\w*').match(s, i); i = m.end()
        else: break
    return i

def fix_site(s, off, kind, mode):
    """off = offset of the cast. Returns (start, end, replacement) or None."""
    if s.startswith('reinterpret_cast', off) or s.startswith('static_cast', off):
        m = re.compile(r'(reinterpret_cast|static_cast)\s*<([^<>]*(?:<[^<>]*>[^<>]*)*)>\s*\(').match(s, off)
        if not m: return None
        close = match_close(s, m.end() - 1, '(', ')')
        inner = s[m.end():close]
        T = m.group(2).strip()
        if kind == "p2i":
            rep = f"fl::PtrToHandle({inner})" if mode == "table" else f"static_cast<{T}>(fl::PtrToHandle({inner}))"
            if mode == "table" and not re.match(r'(unsigned( int)?|uint32_t|int32_t|int|DWORD|UINT)$', T): rep = f"static_cast<{T}>(fl::PtrToHandle({inner}))"
        else:
            rep = f"static_cast<{T}>(fl::HandleToPtr({inner}))" if mode == "table" else f"static_cast<{T}>(fl::HandleToPtr({inner}))"
        return off, close + 1, rep
    m = re.compile(r'\(\s*([^()]*?)\s*\)').match(s, off)
    if not m: return None
    T = m.group(1); end = operand_end(s, m.end())
    if end < 0: return None
    operand = s[m.end():end].strip()
    if kind == "p2i":
        rep = f"({T})fl::PtrToHandle({operand})" if mode == "table" else f"({T})fl::PtrToHandle({operand})"
    else:
        rep = f"({T})fl::HandleToPtr({operand})" if mode == "table" else f"({T})fl::HandleToPtr({operand})"
    return off, end, rep

def main(log):
    pat = re.compile(r'^(/[^:\n]+):(\d+):(\d+): error: cast (?:to smaller integer type|to .+? from smaller integer type)(.*)$', re.M)
    sites = collections.defaultdict(set)
    for m in pat.finditer(open(log, errors="ignore").read()):
        path, line, col, rest = m.group(1), int(m.group(2)), int(m.group(3)), m.group(0)
        rel = path.replace(str(ROOT) + "/", "")
        if rel.startswith("src/platform"): continue
        kind = "p2i" if "to smaller integer type" in rest else "i2p"
        sites[rel].add((line, col, kind))
    total = 0
    for rel, items in sites.items():
        p = ROOT / rel
        text = p.read_text(errors="surrogateescape")
        lines = text.split("\n"); starts = [0]
        for l in lines[:-1]: starts.append(starts[-1] + len(l) + 1)
        edits = []
        for line, col, kind in items:
            off = starts[line - 1] + col - 1
            r = fix_site(text, off, kind, mode_for(rel))
            if r: edits.append(r)
        edits.sort(reverse=True)
        last = len(text) + 1; applied = 0
        for a, b, rep in edits:
            if b > last: continue          # overlaps an outer edit already applied
            text = text[:a] + rep + text[b:]; last = a; applied += 1
        if applied and "fl::" in text and "fl/handles.h" not in text:
            m = re.search(r'^#include[^\n]*\n', text, re.M)
            text = (text[:m.end()] + '#include "fl/handles.h"\n' + text[m.end():]) if m else '#include "fl/handles.h"\n' + text
        p.write_text(text, errors="surrogateescape"); total += applied
        print(f"{rel}: {applied}/{len(items)}")
    print("applied", total)
main(sys.argv[1])

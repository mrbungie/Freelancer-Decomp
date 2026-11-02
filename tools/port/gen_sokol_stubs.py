#!/usr/bin/env python3
"""Regenerates src/platform/render/sokol/sokol_device_stubs.inc from render/IRenderPipeline.h.
Run from the repo root after the interface header changes."""
import re
t = open('src/include/render/IRenderPipeline.h').read()
t = re.sub(r'//[^\n]*', '', t)
out = ["// GENERATED from src/include/render/IRenderPipeline.h by tools/port/gen_sokol_stubs.py -- do not edit.",
       "// Default implementation of every pipeline method: report once, succeed, write no outputs.",
       "// FSokolDevice overrides the ones that are really implemented."]
for name in ['IRenderPipeline', 'IVertexBufferManager', 'IRPDraw', 'IRPIndexBuffer', 'IRPVertexBuffer', 'IGammaControl', 'IRPTexture']:
    m = re.search(r'struct ' + name + r'\s*:\s*([A-Za-z]+)\s*\{(.*?)\n\};', t, re.S)
    for r, n, a in re.findall(r'virtual\s+([\w:\*\s]+?)\s+DACOM_API\s+(\w+)\(([^;]*?)\)\s*(?:const)?\s*=\s*0;', m.group(2), re.S):
        a = ' '.join(a.split())
        out.append(f'virtual {r.strip()} DACOM_API {n}({a}) override {{ return Stub("{name}::{n}"); }}')
open('src/platform/render/sokol/sokol_device_stubs.inc', 'w').write('\n'.join(out) + '\n')

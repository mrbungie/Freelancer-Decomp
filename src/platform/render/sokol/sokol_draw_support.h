// Helpers for the sokol device's fixed-function emulation: vertex layouts, matrices, pixel conversion and the
// uber shader. The fixed-function state (render states, texture stages) is retail Direct3D-8 *semantics* that the
// engine's pipeline contract still speaks; it is evaluated here in one Metal Shading Language shader.
#pragma once
#include <stdint.h>
#include <string.h>
#include <math.h>

namespace fldraw
{
    // ---------------------------------------------------------------- vertex layout from an FVF code
    struct FvfLayout
    {
        unsigned stride = 0;
        int pos = 0;            // 0 none, 3 = xyz, 4 = xyzrhw
        int normal = -1, diffuse = -1, specular = -1;
        int uv[2] = {-1, -1};
        unsigned texCount = 0;
        bool rhw = false;
    };

    inline unsigned TexCoordBytes(unsigned fvf, unsigned set)
    {
        switch ((fvf >> (16 + set * 2)) & 3) { case 0: return 8; case 1: return 12; case 2: return 16; default: return 4; }
    }

    inline FvfLayout MakeLayout(unsigned fvf)
    {
        FvfLayout l;
        unsigned off = 0;
        const unsigned posType = fvf & 0xE;
        if (posType == 4 || (fvf & 0xC) == 0xC) { l.pos = 4; l.rhw = true; off = 16; }
        else if (posType >= 2) { l.pos = 3; off = 12; if (posType > 2) off += 4 * ((posType >> 1) - 1); }
        if (fvf & 0x10) { l.normal = (int)off; off += 12; }
        if (fvf & 0x20) off += 4;                       // point size
        if (fvf & 0x40) { l.diffuse = (int)off; off += 4; }
        if (fvf & 0x80) { l.specular = (int)off; off += 4; }
        l.texCount = (fvf >> 8) & 0xF;
        for (unsigned t = 0; t < l.texCount; ++t)
        {
            if (t < 2) l.uv[t] = (int)off;
            off += TexCoordBytes(fvf, t);
        }
        l.stride = off;
        return l;
    }

    inline unsigned PrimCountFromVerts(uint32_t prim, unsigned n)
    {
        switch (prim)
        {
        case 1: return n;                      // point list
        case 2: return n / 2;                  // line list
        case 3: return n > 0 ? n - 1 : 0;      // line strip
        case 4: return n / 3;                  // triangle list
        case 5: case 6: return n > 1 ? n - 2 : 0;   // strip / fan
        }
        return 0;
    }

    // ---------------------------------------------------------------- matrices (row-vector, Direct3D memory order)
    inline void Identity(float* m) { memset(m, 0, 64); m[0] = m[5] = m[10] = m[15] = 1.0f; }
    inline void Mul(float* out, const float* a, const float* b)   // out = a * b (row vectors: apply a, then b)
    {
        float r[16];
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j] + a[i * 4 + 3] * b[12 + j];
        memcpy(out, r, 64);
    }
    // Engine 3x4 transform (row-major 3x3 rotation, then translation; column-vector) -> D3D matrix; optionally Z-flipped
    // first (the pipeline's view space looks down -Z, Direct3D's down +Z).
    inline void FromAffine(float* v, const float* m12, bool zflip)
    {
        float r[12];
        memcpy(r, m12, sizeof(r));
        if (zflip) { r[6] = -r[6]; r[7] = -r[7]; r[8] = -r[8]; r[11] = -r[11]; }
        v[0] = r[0]; v[4] = r[1]; v[8] = r[2];
        v[1] = r[3]; v[5] = r[4]; v[9] = r[5];
        v[2] = r[6]; v[6] = r[7]; v[10] = r[8];
        v[12] = r[9]; v[13] = r[10]; v[14] = r[11];
        v[3] = v[7] = v[11] = 0.0f; v[15] = 1.0f;
    }
    inline void FromTransposed(float* v, const float* m16)
    {
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c) v[r * 4 + c] = m16[c * 4 + r];
    }

    // ---------------------------------------------------------------- pixels -> BGRA8
    inline void ToBgra8(unsigned pf, unsigned w, unsigned h, const unsigned char* s, unsigned char* d)
    {
        const size_t n = (size_t)w * h;
        switch (pf)
        {
        case 7: memcpy(d, s, n * 4); break;
        case 8: for (size_t i = 0; i < n; ++i) { d[i*4] = s[i*4]; d[i*4+1] = s[i*4+1]; d[i*4+2] = s[i*4+2]; d[i*4+3] = 255; } break;
        case 2: for (size_t i = 0; i < n; ++i) { d[i*4] = s[i*3]; d[i*4+1] = s[i*3+1]; d[i*4+2] = s[i*3+2]; d[i*4+3] = 255; } break;
        case 3: case 4: case 5: case 6:
            for (size_t i = 0; i < n; ++i)
            {
                const unsigned p = s[i*2] | (s[i*2+1] << 8);
                unsigned r, g, b, a = 255;
                if (pf == 3) { r = (p >> 11) & 31; g = (p >> 5) & 63; b = p & 31; r = r * 255 / 31; g = g * 255 / 63; b = b * 255 / 31; }
                else if (pf == 5) { a = ((p >> 12) & 15) * 17; r = ((p >> 8) & 15) * 17; g = ((p >> 4) & 15) * 17; b = (p & 15) * 17; }
                else { if (pf == 6) a = (p & 0x8000) ? 255 : 0; r = ((p >> 10) & 31) * 255 / 31; g = ((p >> 5) & 31) * 255 / 31; b = (p & 31) * 255 / 31; }
                d[i*4] = (unsigned char)b; d[i*4+1] = (unsigned char)g; d[i*4+2] = (unsigned char)r; d[i*4+3] = (unsigned char)a;
            }
            break;
        case 1: for (size_t i = 0; i < n; ++i) { d[i*4] = d[i*4+1] = d[i*4+2] = s[i]; d[i*4+3] = 255; } break;
        default: memset(d, 0xFF, n * 4); break;
        }
    }

    // ---------------------------------------------------------------- uniform blocks (all float4-based: no padding rules)
    struct VsUniforms
    {
        float mvp[16];
        float world[16];
        float misc[4];      // vpW, vpH, rhw, lightCount
        float flags[4];     // lit, diffuse source is vertex colour, ambient source is vertex colour, hasDiffuse
        float gamb[4];      // global ambient (D3DRS_AMBIENT)
        float mat[4][4];    // diffuse, ambient, emissive, unused
        float lights[8][6][4];  // per light: diffuse, ambient, pos+range, dir+type, atten0..2+falloff, cos(theta/2), cos(phi/2)
    };
    struct FsUniforms { float tfactor[4]; float ctl[4]; float st[2][2][4]; };   // ctl: alphaTest, func, ref, hasDiffuse

    static const char* kShaderMsl = R"MSL(
#include <metal_stdlib>
using namespace metal;
struct VSU { float4x4 mvp; float4x4 world; float4 misc; float4 flags; float4 gamb; float4 mat[4]; float4 lights[48]; };
struct FSU { float4 tf; float4 ctl; float4 s0a; float4 s0b; float4 s1a; float4 s1b; };
struct VIn { float4 pos [[attribute(0)]]; float3 nrm [[attribute(1)]]; float4 col [[attribute(2)]];
             float4 spec [[attribute(3)]]; float2 uv0 [[attribute(4)]]; float2 uv1 [[attribute(5)]]; };
struct VOut { float4 pos [[position]]; float4 col; float4 spec; float2 uv0; float2 uv1; };
vertex VOut vs_main(VIn in [[stage_in]], constant VSU& u [[buffer(0)]]) {
  VOut o;
  if (u.misc.z > 0.5) {
    float rhw = in.pos.w > 0.0 ? in.pos.w : 1.0;
    float w = 1.0 / rhw;
    float2 ndc = float2(in.pos.x / u.misc.x * 2.0 - 1.0, 1.0 - in.pos.y / u.misc.y * 2.0);
    o.pos = float4(ndc * w, in.pos.z * w, w);
  } else {
    o.pos = u.mvp * float4(in.pos.xyz, 1.0);
  }
  float4 vcol = in.col.zyxw;
  o.col = vcol;
  if (u.flags.x > 0.5) {
    float3 wp = (u.world * float4(in.pos.xyz, 1.0)).xyz;
    float3 n = normalize((u.world * float4(in.nrm, 0.0)).xyz);
    bool hasD = u.flags.w > 0.5;
    float4 md = (u.flags.y > 0.5 && hasD) ? vcol : u.mat[0];
    float4 ma = (u.flags.z > 0.5 && hasD) ? vcol : u.mat[1];
    float3 amb = u.gamb.xyz;
    float3 dif = float3(0.0);
    int cnt = int(u.misc.w + 0.5);
    for (int i = 0; i < cnt; ++i) {
      float4 ld = u.lights[i * 6 + 0], la = u.lights[i * 6 + 1], lp = u.lights[i * 6 + 2], lr = u.lights[i * 6 + 3];
      float4 at = u.lights[i * 6 + 4], sp = u.lights[i * 6 + 5];
      int type = int(lr.w + 0.5);
      float3 L; float att = 1.0;
      if (type == 3) { L = normalize(-lr.xyz); }
      else {
        float3 d = lp.xyz - wp; float dist = length(d);
        L = d / max(dist, 1e-6);
        if (dist > lp.w) att = 0.0;
        else att = 1.0 / max(at.x + at.y * dist + at.z * dist * dist, 1e-6);
        if (type == 2) {
          float rho = dot(-L, normalize(lr.xyz));
          if (rho <= sp.y) att = 0.0;
          else if (rho < sp.x) att *= pow((rho - sp.y) / max(sp.x - sp.y, 1e-6), at.w);
        }
      }
      amb += la.xyz * att;
      dif += ld.xyz * (max(dot(n, L), 0.0) * att);
    }
    float3 c = u.mat[2].xyz + ma.xyz * amb + md.xyz * dif;
    o.col = float4(clamp(c, 0.0, 1.0), md.w);
  }
  o.spec = in.spec.zyxw;
  o.uv0 = in.uv0; o.uv1 = in.uv1;
  return o;
}
float4 targ(int a, float4 cur, float4 diff, float4 tex, float4 tf, float4 spec) {
  int k = a & 15; float4 v = cur;
  if (k == 0) v = diff; else if (k == 1) v = cur; else if (k == 2) v = tex; else if (k == 3) v = tf; else if (k == 4) v = spec;
  if (a & 32) v = float4(v.w);
  if (a & 16) v = float4(1.0) - v;
  return v;
}
float4 applyop(int op, float4 a1, float4 a2, float4 cur, float4 diff, float4 tex, float4 tf) {
  switch (op) {
    case 2: return a1;
    case 3: return a2;
    case 4: return a1 * a2;
    case 5: return a1 * a2 * 2.0;
    case 6: return a1 * a2 * 4.0;
    case 7: return a1 + a2;
    case 8: return a1 + a2 - 0.5;
    case 9: return (a1 + a2 - 0.5) * 2.0;
    case 10: return a1 - a2;
    case 11: return a1 + a2 - a1 * a2;
    case 12: return mix(a2, a1, diff.w);
    case 13: return mix(a2, a1, tex.w);
    case 14: return mix(a2, a1, tf.w);
    case 15: return a1 + a2 * (1.0 - tex.w);
    case 16: return mix(a2, a1, cur.w);
  }
  return a1 * a2;
}
float4 stage(float4 a, float4 b, float4 cur, float4 diff, float4 tex, float4 tf, float4 spec) {
  int cop = int(a.x + 0.5);
  if (cop <= 1) return cur;
  float4 c1 = targ(int(a.y + 0.5), cur, diff, tex, tf, spec);
  float4 c2 = targ(int(a.z + 0.5), cur, diff, tex, tf, spec);
  float3 rgb = applyop(cop, c1, c2, cur, diff, tex, tf).xyz;
  int aop = int(a.w + 0.5);
  float al = cur.w;
  if (aop > 1) {
    float4 x1 = targ(int(b.x + 0.5), cur, diff, tex, tf, spec);
    float4 x2 = targ(int(b.y + 0.5), cur, diff, tex, tf, spec);
    al = applyop(aop, x1, x2, cur, diff, tex, tf).w;
  }
  return float4(rgb, al);
}
fragment float4 fs_main(VOut in [[stage_in]], constant FSU& u [[buffer(0)]],
                        texture2d<float> t0 [[texture(0)]], sampler s0 [[sampler(0)]],
                        texture2d<float> t1 [[texture(1)]], sampler s1 [[sampler(1)]]) {
  float4 diff = u.ctl.w > 0.5 ? in.col : float4(1.0);
  float4 cur = diff;
  float2 uv0 = u.s0b.z > 0.5 ? in.uv1 : in.uv0;
  float2 uv1 = u.s1b.z > 0.5 ? in.uv1 : in.uv0;
  cur = stage(u.s0a, u.s0b, cur, diff, t0.sample(s0, uv0), u.tf, in.spec);
  cur = stage(u.s1a, u.s1b, cur, diff, t1.sample(s1, uv1), u.tf, in.spec);
  if (u.ctl.x > 0.5) {
    int f = int(u.ctl.y + 0.5); float r = u.ctl.z; float a = cur.w; bool pass = true;
    if (f == 1) pass = false; else if (f == 2) pass = a < r; else if (f == 3) pass = a == r; else if (f == 4) pass = a <= r;
    else if (f == 5) pass = a > r; else if (f == 6) pass = a != r; else if (f == 7) pass = a >= r;
    if (!pass) discard_fragment();
  }
  return cur;
}
)MSL";
}

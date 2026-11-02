#include "fl/text.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <algorithm>

namespace fl { namespace text
{
    namespace { IRasterizer* G_Rasterizer = nullptr; }
    void SetRasterizer(IRasterizer* r) { G_Rasterizer = r; }
    IRasterizer* GetRasterizer() { return G_Rasterizer; }

    Bitmap* Bitmap::LoadBmp(const char* path)
    {
        if (!path) return nullptr;
        std::string p(path);
        for (char& c : p) if (c == '\\') c = '/';
        FILE* f = fopen(p.c_str(), "rb");
        if (!f) return nullptr;
        fseek(f, 0, SEEK_END); long size = ftell(f); fseek(f, 0, SEEK_SET);
        std::vector<uint8_t> d((size_t)(size > 0 ? size : 0));
        if (size <= 0 || fread(d.data(), 1, d.size(), f) != d.size()) { fclose(f); return nullptr; }
        fclose(f);
        auto u16 = [&](size_t o) { return (uint32_t)d[o] | ((uint32_t)d[o + 1] << 8); };
        auto u32 = [&](size_t o) { return u16(o) | (u16(o + 2) << 16); };
        if (d.size() < 54 || d[0] != 'B' || d[1] != 'M') return nullptr;
        const uint32_t dataOff = u32(10), hdr = u32(14);
        if (hdr < 40) return nullptr;
        const int w = (int)u32(18); int h = (int)u32(22);
        const unsigned bpp = u16(28); const uint32_t comp = u32(30);
        if (w <= 0 || h == 0 || comp != 0 || (bpp != 1 && bpp != 4 && bpp != 8 && bpp != 24 && bpp != 32)) return nullptr;
        const bool bottomUp = h > 0; if (h < 0) h = -h;
        uint32_t colors = u32(46); if (colors == 0 && bpp <= 8) colors = 1u << bpp;
        const size_t palOff = 14 + hdr;
        const size_t rowBytes = (((size_t)w * bpp + 31) / 32) * 4;
        if (dataOff + rowBytes * (size_t)h > d.size()) return nullptr;
        Bitmap* b = new Bitmap(); b->w = w; b->h = h; b->px.resize((size_t)w * h);
        for (int y = 0; y < h; ++y)
        {
            const uint8_t* row = &d[dataOff + rowBytes * (size_t)(bottomUp ? h - 1 - y : y)];
            for (int x = 0; x < w; ++x)
            {
                uint32_t r, g, bl;
                if (bpp >= 24) { const uint8_t* q = row + (size_t)x * (bpp / 8); bl = q[0]; g = q[1]; r = q[2]; }
                else
                {
                    unsigned idx = bpp == 8 ? row[x] : bpp == 4 ? ((row[x / 2] >> ((x & 1) ? 0 : 4)) & 15) : ((row[x / 8] >> (7 - (x & 7))) & 1);
                    const size_t o = palOff + (size_t)idx * 4;
                    if (idx >= colors || o + 3 > d.size()) { r = g = bl = 0; } else { bl = d[o]; g = d[o + 1]; r = d[o + 2]; }
                }
                b->px[(size_t)y * w + x] = (r << 16) | (g << 8) | bl;
            }
        }
        return b;
    }

    Surface::Surface(int w, int h) : w_(w > 0 ? w : 0), h_(h > 0 ? h : 0), px_((size_t)w_ * h_, 0) {}
    void Surface::Fill(uint32_t rgb) { std::fill(px_.begin(), px_.end(), rgb & 0xFFFFFF); }
    void Surface::SetClip(int l, int t, int r, int b) { clip_ = true; cl_ = l; ct_ = t; cr_ = r; cb_ = b; }
    void Surface::ClearClip() { clip_ = false; }
    bool Surface::GetClip(int* l, int* t, int* r, int* b) const
    {
        if (!clip_) return false;
        *l = cl_; *t = ct_; *r = cr_; *b = cb_; return true;
    }

    void Surface::DrawText(const IFont* font, const wchar_t* text, int len, int x, int y, uint32_t rgb)
    {
        if (!font || !text || len <= 0 || w_ == 0 || h_ == 0) return;
        int tw = 0, th = 0;
        if (!font->Extent(text, len, &tw, &th) || tw <= 0 || th <= 0) return;
        tw += 2;   // slack for italic overhang
        std::vector<uint16_t> cov((size_t)tw * th);
        font->DrawGlyphs16(text, len, 0, 0, cov.data(), tw, th, tw);
        const int x0 = clip_ ? std::max(cl_, 0) : 0, y0 = clip_ ? std::max(ct_, 0) : 0;
        const int x1 = clip_ ? std::min(cr_, w_) : w_, y1 = clip_ ? std::min(cb_, h_) : h_;
        const uint32_t sr = (rgb >> 16) & 255, sg = (rgb >> 8) & 255, sb = rgb & 255;
        for (int j = 0; j < th; ++j)
        {
            const int dy = y + j; if (dy < y0 || dy >= y1) continue;
            for (int i = 0; i < tw; ++i)
            {
                const int dx = x + i; if (dx < x0 || dx >= x1) continue;
                const uint32_t g = cov[(size_t)j * tw + i] & 31; if (!g) continue;
                uint32_t& d = px_[(size_t)dy * w_ + dx];
                const uint32_t dr = (d >> 16) & 255, dg = (d >> 8) & 255, db = d & 255;
                const uint32_t r = (sr * g + dr * (31 - g)) / 31, gg = (sg * g + dg * (31 - g)) / 31, b = (sb * g + db * (31 - g)) / 31;
                d = (r << 16) | (gg << 8) | b;
            }
        }
    }

    void Surface::StretchBlt(const Bitmap& src, int dx, int dy, int dw, int dh, int sx, int sy, int sw, int sh)
    {
        if (dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0 || src.w <= 0) return;
        const int x0 = clip_ ? std::max(cl_, 0) : 0, y0 = clip_ ? std::max(ct_, 0) : 0;
        const int x1 = clip_ ? std::min(cr_, w_) : w_, y1 = clip_ ? std::min(cb_, h_) : h_;
        for (int j = 0; j < dh; ++j)
        {
            const int py = dy + j; if (py < y0 || py >= y1) continue;
            const int srcY = std::min(sy + j * sh / dh, src.h - 1);
            for (int i = 0; i < dw; ++i)
            {
                const int px = dx + i; if (px < x0 || px >= x1) continue;
                const int srcX = std::min(sx + i * sw / dw, src.w - 1);
                px_[(size_t)py * w_ + px] = src.px[(size_t)srcY * src.w + srcX];
            }
        }
    }

    void Surface::ReadX1R5G5B5(uint8_t* dst, int pitchBytes) const
    {
        for (int y = 0; y < h_; ++y)
        {
            uint16_t* row = reinterpret_cast<uint16_t*>(dst + (size_t)y * pitchBytes);
            for (int x = 0; x < w_; ++x)
            {
                const uint32_t c = px_[(size_t)y * w_ + x];
                row[x] = (uint16_t)((((c >> 19) & 31) << 10) | (((c >> 11) & 31) << 5) | ((c >> 3) & 31));
            }
        }
    }
}}

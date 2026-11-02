// fl::text::IRasterizer on CoreText / CoreGraphics (macOS). Objective-C++, ARC.
#include "fl/text.h"
#import <CoreText/CoreText.h>
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#include <math.h>
#include <string.h>
#include <string>
#include <vector>

namespace
{
    CFStringRef MakeString(const wchar_t* text, int len)
    {
        std::vector<UniChar> u;
        u.reserve((size_t)len);
        for (int i = 0; i < len; ++i)
        {
            uint32_t c = (uint32_t)text[i];
            if (c >= 0x10000) { c -= 0x10000; u.push_back((UniChar)(0xD800 + (c >> 10))); u.push_back((UniChar)(0xDC00 + (c & 0x3FF))); }
            else u.push_back((UniChar)c);
        }
        return CFStringCreateWithCharacters(kCFAllocatorDefault, u.data(), (CFIndex)u.size());
    }

    class CtFont final : public fl::text::IFont
    {
        CTFontRef font_ = nullptr;
        fl::text::FontMetrics m_;
    public:
        CtFont(const char* face, int cell, bool bold, bool italic)
        {
            if (cell < 1) cell = 1;
            // Pick the point size whose ascent+descent equals the requested cell height (GDI positive lfHeight).
            CFStringRef name = CFStringCreateWithCString(kCFAllocatorDefault, face && *face ? face : "Helvetica", kCFStringEncodingUTF8);
            CTFontRef probe = CTFontCreateWithName(name, 100.0, nullptr);
            const double cell100 = CTFontGetAscent(probe) + CTFontGetDescent(probe);
            CFRelease(probe);
            const double size = cell100 > 0 ? cell * 100.0 / cell100 : cell;
            CTFontRef base = CTFontCreateWithName(name, size, nullptr);
            CFRelease(name);
            CTFontSymbolicTraits traits = (bold ? kCTFontBoldTrait : 0) | (italic ? kCTFontItalicTrait : 0);
            font_ = traits ? CTFontCreateCopyWithSymbolicTraits(base, size, nullptr, traits, traits) : nullptr;
            if (font_) CFRelease(base); else font_ = base;

            m_.ascent = (int)ceil(CTFontGetAscent(font_));
            m_.height = cell;
            m_.descent = cell - m_.ascent;
            if (m_.descent < 0) { m_.descent = 0; m_.ascent = cell; }
            m_.internalLeading = cell - (int)floor(size + 0.5);
            if (m_.internalLeading < 0) m_.internalLeading = 0;
            m_.externalLeading = (int)floor(CTFontGetLeading(font_) + 0.5);
            m_.aveCharWidth = CharWidth(L'x');
            CGRect bb = CTFontGetBoundingBox(font_);
            m_.maxCharWidth = (int)ceil(bb.size.width);
        }
        ~CtFont() override { if (font_) CFRelease(font_); }

        fl::text::FontMetrics Metrics() const override { return m_; }

        int CharWidth(wchar_t ch) const override
        {
            wchar_t s[1] = {ch};
            int w = 0, h = 0;
            Extent(s, 1, &w, &h);
            return w;
        }

        CTLineRef MakeLine(const wchar_t* text, int len) const
        {
            CFStringRef str = MakeString(text, len);
            CFTypeRef keys[] = {kCTFontAttributeName, kCTForegroundColorFromContextAttributeName};
            CFTypeRef vals[] = {font_, kCFBooleanTrue};
            CFDictionaryRef attrs = CFDictionaryCreate(kCFAllocatorDefault, (const void**)keys, (const void**)vals, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
            CFAttributedStringRef as = CFAttributedStringCreate(kCFAllocatorDefault, str, attrs);
            CTLineRef line = CTLineCreateWithAttributedString(as);
            CFRelease(as); CFRelease(attrs); CFRelease(str);
            return line;
        }

        bool Extent(const wchar_t* text, int len, int* width, int* height) const override
        {
            if (len < 0) len = (int)wcslen(text);
            double w = 0;
            if (len > 0)
            {
                CTLineRef line = MakeLine(text, len);
                w = CTLineGetTypographicBounds(line, nullptr, nullptr, nullptr);
                CFRelease(line);
            }
            *width = (int)floor(w + 0.5);
            *height = m_.height;
            return true;
        }

        void DrawGlyphs16(const wchar_t* text, int len, int x, int y, uint16_t* dst, int w, int h, int pitch) const override
        {
            for (int r = 0; r < h; ++r) memset(dst + (size_t)r * pitch, 0, (size_t)w * 2);
            if (!text) return;
            if (len < 0) len = (int)wcslen(text);
            if (len <= 0 || w <= 0 || h <= 0) return;
            std::vector<uint8_t> grey((size_t)w * h, 0);
            CGColorSpaceRef cs = CGColorSpaceCreateDeviceGray();
            CGContextRef ctx = CGBitmapContextCreate(grey.data(), (size_t)w, (size_t)h, 8, (size_t)w, cs, kCGImageAlphaNone);
            CGColorSpaceRelease(cs);
            if (!ctx) return;
            CGContextSetGrayFillColor(ctx, 1.0, 1.0);
            CGContextSetShouldAntialias(ctx, true);
            CGContextSetShouldSmoothFonts(ctx, false);
            CTLineRef line = MakeLine(text, len);
            // Memory row 0 is the top; CG's origin is bottom-left. Baseline sits `ascent` below the cell top.
            CGContextSetTextPosition(ctx, (CGFloat)x, (CGFloat)(h - (y + m_.ascent)));
            CTLineDraw(line, ctx);
            CFRelease(line);
            CGContextRelease(ctx);
            for (int r = 0; r < h; ++r)
                for (int c = 0; c < w; ++c)
                {
                    const unsigned g = grey[(size_t)r * w + c] >> 3;   // 0..31
                    dst[(size_t)r * pitch + c] = (uint16_t)((g << 10) | (g << 5) | g);
                }
        }
    };

    class CtRasterizer final : public fl::text::IRasterizer
    {
    public:
        fl::text::IFont* CreateFont(const char* face, int cell, bool bold, bool italic, bool) override
        {
            return new CtFont(face, cell, bold, italic);
        }
    };
}

fl::text::IRasterizer* FlCreateCoreTextRasterizer() { return new CtRasterizer(); }

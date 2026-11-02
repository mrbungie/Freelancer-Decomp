// Text rasterizer: the platform's answer to "give me a font of this face/height, measure a string, draw it".
// The game's font code (Client/font_imp.cpp, font_atlas.cpp) calls this directly; a backend (CoreText on macOS, see
// render/sokol/coretext_text.mm) implements it. Heights follow the retail convention (GDI lfHeight > 0): the CELL height
// (ascent + descent) in pixels.
#pragma once
#include <stdint.h>

namespace fl { namespace text
{
    struct FontMetrics
    {
        int height = 0;           // ascent + descent (GDI tmHeight)
        int ascent = 0;
        int descent = 0;
        int internalLeading = 0;
        int externalLeading = 0;
        int aveCharWidth = 0;
        int maxCharWidth = 0;
    };

    class IFont
    {
    public:
        virtual ~IFont() {}
        virtual FontMetrics Metrics() const = 0;
        // Advance of one character in whole pixels (GDI GetCharWidth).
        virtual int CharWidth(wchar_t ch) const = 0;
        // Width / height of a string (GDI GetTextExtentPoint).
        virtual bool Extent(const wchar_t* text, int len, int* width, int* height) const = 0;
        // Clears dst (w x h pixels, pitch in pixels) to 0 and draws `text` white-on-black with its top-left at (x, y)
        // (x/y may be negative: the text is positioned inside a larger virtual image). 16-bit A1R5G5B5-style pixels:
        // grey level g (0..31) is stored as (g << 10) | (g << 5) | g.
        virtual void DrawGlyphs16(const wchar_t* text, int len, int x, int y, uint16_t* dst, int w, int h, int pitch) const = 0;
    };

    class IRasterizer
    {
    public:
        virtual ~IRasterizer() {}
        virtual IFont* CreateFont(const char* face, int cellHeight, bool bold, bool italic, bool underline) = 0;
    };

    void SetRasterizer(IRasterizer* r);
    IRasterizer* GetRasterizer();   // null when the build has no text backend
}}

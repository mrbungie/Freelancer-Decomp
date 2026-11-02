// Freelancer Decomp: a reimplementation of the Freelancer (2003) game client and engine,
// faithful to the behaviour of the original game.
// This file was reconstructed by reverse engineering the binaries of Freelancer 1.1,
// developed by Digital Anvil.
// Author: Giperion.

#pragma once

// Retail PixelFormat helpers (0x005b7000..0x005b70d7) and their two .data tables. Every
// BigImage descriptor, texture-format query and display-mode record in the exe identifies a
// pixel format by the small integer id these tables index.
//
//   DAT_00615e70 -- 0x16 D3DFORMAT codes, indexed by id (FUN_005b7000 returns the array,
//                   FUN_005b7010 maps a D3DFORMAT back to its id).
//   DAT_00615ec8 -- 15 descriptors of 0x4c bytes, indexed by id (FUN_005b7030).
//
// The only code references to either table are those accessors (.text:005b7000 / 005b7039 /
// 005b7045); nothing in the PE writes them after load.

// One colour channel of a descriptor.
struct FPixelFormatChannel
{
    uint32_t Bits;  // +0x00
    uint32_t Mask;  // +0x04
    uint32_t Shift; // +0x08
    uint32_t Loss;  // +0x0c -- 8 - Bits
};

// DAT_00615ec8 entry.
struct FPixelFormatDesc
{
    uint32_t Id;             // +0x00
    uint32_t Bpp;            // +0x04
    FPixelFormatChannel Red;      // +0x08
    FPixelFormatChannel Green;    // +0x18
    FPixelFormatChannel Blue;     // +0x28
    FPixelFormatChannel Alpha;    // +0x38
    uint32_t HasAlpha;       // +0x48 -- FUN_005b7050 reads its low byte
};

static_assert(sizeof(FPixelFormatDesc) == 0x4c, "retail PixelFormat descriptor is 0x4c bytes");

// FUN_005b7000 -- DAT_00615e70, the 0x16-entry D3DFORMAT array.
uint32_t* PixelFormatGetD3DFormats();

// FUN_005b7010 (cdecl) -- the id whose D3DFORMAT equals D3DFormat, or 0 when none does. Every
// caller seeds a format id with FUN_005b7010(0), i.e. id 0.
int PixelFormatFromD3D(uint32_t D3DFormat);

// FUN_005b7030 (cdecl) -- the descriptor of Id; an Id above 14 (signed compare) gives entry 0.
FPixelFormatDesc* PixelFormatGetDesc(int Id);

// FUN_005b7050 (thiscall on the id, RET 0x14) -- the first of the first nine descriptors with
// this Bpp, HasAlpha == (AlphaBits != 0), and matching channel widths. Stores its id in *OutId
// and returns true; otherwise stores FUN_005b7010(0) and returns false.
//
// Retail quirk kept: the blue width is compared against the descriptor's RED width
// (.text:005b709c CMP EDX,ECX with ECX still [EAX+8]), not its blue width.
bool PixelFormatFind(uint32_t* OutId, uint32_t Bpp, uint32_t RedBits,
                     uint32_t GreenBits, uint32_t BlueBits, uint32_t AlphaBits);

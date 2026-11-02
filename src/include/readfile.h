// Freelancer Decomp: a reimplementation of the Freelancer (2003) game client and engine,
// faithful to the behaviour of the original game.
// This file was reconstructed by reverse engineering the binaries of Freelancer 1.1,
// developed by Digital Anvil.
// Author: Giperion.

#pragma once

// Public ABI of ReadFile.dll -- the DACOM FileSystem component (DOS / UTF / MEM)
// and SearchPath. Every module that touches game data calls through this; it is
// declared here and nowhere else (docs/re/cross_dll_protocol.md).

#include "dacom.h"

#include <cstddef>

// Digital Anvil FileSystem AGGDESC (size 0x30 or 0x34). DALib::OpenData / ReadFile.dll.c.
struct FFileSystemDesc
{
    uint32_t Size;              // +0x00  0x30 or 0x34
    const char* ComponentName;       // +0x04  "FileSystem"
    const char* Implementation;      // +0x08  "DOS" / "UTF" / "MEM"
    const char* Path;                // +0x0C
    uint32_t Access;            // +0x10  GENERIC_READ 0x80000000
    uint32_t ShareMode;         // +0x14  FILE_SHARE_READ
    void* Security;                  // +0x18
    uint32_t Creation;          // +0x1C  OPEN_EXISTING 3
    uint32_t Flags;             // +0x20  0x08000080
    void* TemplateFile;              // +0x24
    IDAComponent* Parent;            // +0x28
    uint32_t Extra;             // +0x2C
    uint32_t Field30;           // +0x30  0xFFFFFFFF when Size==0x34
    void* MemBuffer;                 // +0x34  MEM size 0x40 only (FUN_06b777f0)
    uint32_t MemSize;           // +0x38
    uint32_t MemFlags;          // +0x3C  bit0=wrap pointer, bit1=flag+1
};

// Callers fill `Size` (0x30 / 0x34 for DOS and UTF, 0x40 for MEM) and the callee reads
// only that much, so the DOS-sized prefix layout is the invariant that matters.
#if defined(_M_IX86) || defined(__i386__)
static_assert(offsetof(FFileSystemDesc, Field30) == 0x30, "DOS/UTF desc prefix is 0x34");
static_assert(sizeof(FFileSystemDesc) == 0x40, "MEM tail extends the desc to 0x40");
#endif

// File-system instances also expose their component factory.
struct IFileSystem : IComponentFactory
{
    virtual int DACOM_API CloseFile(uint32_t Handle) = 0;                          // +0x10
    virtual int DACOM_API Read(uint32_t Handle, void* Buffer, uint32_t Size,
                               uint32_t* BytesRead, uint32_t Extra) = 0;      // +0x14
    virtual int DACOM_API Write(uint32_t Handle, const void* Buffer, uint32_t Size,
                                 uint32_t* BytesWritten, const void* PositionOverride) = 0; // formerly Slot18
    virtual int DACOM_API RejectOperation1C(uint32_t A1, uint32_t A2, uint32_t A3,
                                 uint32_t A4) = 0; // formerly Slot1C; TODO: original operation contract.
    virtual int DACOM_API SeekFile(uint32_t Handle, uint32_t OffLow, uint32_t* OffHighOut,
                                  uint32_t Origin) = 0; // formerly Slot20
    virtual int DACOM_API SetEndOfFile(uint32_t A1) = 0; // formerly Slot24
    virtual uint32_t DACOM_API GetSize(uint32_t Handle, uint32_t* HighOut) = 0; // +0x28
    virtual int DACOM_API ReturnOne2C(uint32_t A1, uint32_t A2, uint32_t A3,
                                 uint32_t A4, uint32_t A5) = 0; // formerly Slot2C; TODO: original operation contract.
    virtual int DACOM_API ReturnOne30(uint32_t A1, uint32_t A2, uint32_t A3,
                                 uint32_t A4, uint32_t A5) = 0; // formerly Slot30; TODO: original operation contract.
    virtual int DACOM_API GetFileTimes(uint32_t Handle, FILETIME* CreationOut, FILETIME* AccessOut,
                                  FILETIME* WriteOut) = 0; // formerly Slot34
    virtual int DACOM_API RejectOperation38(uint32_t A1, uint32_t A2, uint32_t A3,
                                 uint32_t A4) = 0; // formerly Slot38; TODO: original operation contract.
    virtual int DACOM_API CreateMapping(uint32_t A1, uint32_t A2, uint32_t A3,
                                 uint32_t A4, uint32_t A5, uint32_t A6) = 0; // formerly Slot3C
    virtual int DACOM_API MapFileView(uint32_t A1, uint32_t A2, uint32_t A3,
                                 uint32_t A4, uint32_t A5) = 0; // formerly Slot40
    virtual int DACOM_API UnmapFileView(uint32_t A1) = 0; // formerly Slot44
    virtual int DACOM_API FindFirst(const char* Pattern, WIN32_FIND_DATAA* FindData) = 0; // formerly Slot48
    virtual int DACOM_API FindNext(uint32_t Handle, WIN32_FIND_DATAA* FindData) = 0; // formerly Slot4C
    virtual int DACOM_API FindClose(uint32_t Handle) = 0; // formerly Slot50
    virtual int DACOM_API RejectOperation54(const char* Name, uint32_t Extra) = 0; // formerly Slot54; TODO: original operation contract.
    virtual int DACOM_API RejectOperation58(uint32_t A1) = 0; // formerly Slot58; TODO: original operation contract.
    virtual int DACOM_API GetCurrentPath(uint32_t MaxChars, char* Out) = 0;        // +0x5C
    virtual int DACOM_API OpenNode(const char* Name) = 0;                               // +0x60
    virtual int DACOM_API RejectOperation64(uint32_t A1) = 0; // formerly Slot64; TODO: original operation contract.
    virtual int DACOM_API RejectOperation68(uint32_t A1, uint32_t A2, uint32_t A3) = 0; // formerly Slot68; TODO: original operation contract.
    virtual int DACOM_API RejectOperation6C(uint32_t A1, uint32_t A2) = 0; // formerly Slot6C; TODO: original operation contract.
    virtual int DACOM_API GetPathAttributes(const char* Path) = 0; // formerly Slot70
    virtual int DACOM_API RejectOperation74(uint32_t A1, uint32_t A2) = 0; // formerly Slot74; TODO: original operation contract.
    virtual int DACOM_API GetFileSystemError() = 0; // formerly Slot78
    virtual int DACOM_API OpenByDesc(FFileSystemDesc* Desc) = 0;                        // +0x7C
    virtual int DACOM_API GetFilePosition(uint32_t Handle, uint32_t* HighOut) = 0; // formerly Slot80
    virtual void DACOM_API GetFileSysPath(char* Out, uint32_t MaxChars) = 0;       // +0x84
    virtual int DACOM_API GetAccessFlags() = 0; // formerly Slot88
    virtual int DACOM_API GetParentFileSystem(IFileSystem** Out) = 0; // formerly Slot8C
    virtual int DACOM_API FailOperation90(uint32_t A1, uint32_t A2) = 0; // formerly Slot90; TODO: original operation contract.
    virtual int DACOM_API FailOperation94(uint32_t A1, uint32_t A2) = 0; // formerly Slot94; TODO: original operation contract.
    virtual int DACOM_API FailOperation98(uint32_t A1, uint32_t A2, uint32_t A3,
                                 uint32_t A4, uint32_t A5) = 0; // formerly Slot98; TODO: original operation contract.
    virtual int DACOM_API FailOperation9C(uint32_t A1, uint32_t A2, uint32_t A3,
                                 uint32_t A4, uint32_t A5) = 0; // formerly Slot9C; TODO: original operation contract.
    virtual int DACOM_API InvokeLockedCallback(uint32_t A1, uint32_t A2, uint32_t A3) = 0; // formerly SlotA0
    virtual int DACOM_API ResolvePath(char* Dest, const char* Src,
                                 uint32_t DestChars) = 0; // formerly SlotA4
};

// Access to a memory file system's underlying buffer.
struct IDataBuffer : IDAComponent
{
    virtual GENRESULT DACOM_API GetBuffer(void** OutPtr, uint32_t* OutSize) = 0; // +0x0C
};

struct ISearchPath : IComponentFactory
{
    virtual int DACOM_API SetPath(const char* PathList) = 0;                            // +0x10
    virtual int DACOM_API GetPath(char* Dest, uint32_t MaxChars) = 0; // formerly Slot14
};

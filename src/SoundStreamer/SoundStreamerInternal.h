// Freelancer Decomp: a reimplementation of the Freelancer (2003) game client and engine,
// faithful to the behaviour of the original game.
// This file was reconstructed by reverse engineering the binaries of Freelancer 1.1,
// developed by Digital Anvil.
// Author: Giperion.

#pragma once

#include "dacom.h"
#include "soundstreamer.h"
#include "audio/dsound8.h"
#include <mmreg.h>
#include <msacm.h>
#include <stddef.h>

static const uint32_t kAggDescSize = 0x14;
static const uint32_t kRegisterPriority = 0x40000000;

static const char kComponentName[] = "SoundStreamer";

static const unsigned int kFdumpError = 0x100001;
static const unsigned int kFdumpWarning = 0x100002;
static const unsigned int kFdumpNotice = 0x100003;

// .data:06efe070: __FILE__ of the StreamingSound handle code (FDUMP sites 06ef101d, 17ec, 18f6,
// 1b0c, 1b3d, 1bc7, 1c7f, 206d).
static const char kRetailSource[] =
    "C:\\work\\builds\\dalibs\\dalibs-build\\build\\Src\\SoundStreamer\\NewStream.cpp";
// .data:06efe320: __FILE__ of the streamer object code (device init 06ef2e00 and CreateStream
// 06ef3060: FDUMP sites 06ef2e80/2e9a, 2efe/2f18, 2fc5, 2ff6, 303c, 31ae).
static const char kStreamerSource[] =
    "C:\\work\\builds\\dalibs\\dalibs-build\\build\\Src\\SoundStreamer\\SoundStreamer.cpp";
static const char kFdumpErrorFmt[] = "%s(%d) : ERROR:General:%s";
static const char kFdumpWarningFmt[] = "%s(%d) : WARNING:General:%s";
static const char kFdumpNoticeFmt[] = "%s(%d) : NOTICE:General:%s";

static const GUID kClsidDirectSound = {
    0x47D4D946, 0x62E8, 0x11CF, {0x93, 0xBC, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
static const GUID kIidDirectSoundRetail = {
    0x279AFA83, 0x4981, 0x11CE, {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}};

struct FStreamerAggDesc
{
    uint32_t Size;
    const char* Name;
    IDAComponent* Outer;
    IDAComponent** InnerOut;
    const char* Extra;
};

class FSoundStreamer;

class FStreamerInner final : public IDAComponent
{
public:
    FStreamerInner();

    GENRESULT DACOM_API QueryInterface(const char* Iid, void** Out) override;
    uint32_t DACOM_API AddRef() override;
    uint32_t DACOM_API Release() override;

    uint32_t RefCount;
    FSoundStreamer* Self;
};

class FSoundStreamerFactory final : public IComponentFactory
{
public:
    FSoundStreamerFactory();

    GENRESULT DACOM_API QueryInterface(const char* Iid, void** Out) override;
    uint32_t DACOM_API AddRef() override;
    uint32_t DACOM_API Release() override;
    GENRESULT DACOM_API CreateInstance(AGGDESC* Desc, void** Instance) override;

    uint32_t RefCount;
    const char* Name;
};

// Heap handle — retail StreamingSound 0x110 (FUN_06ef1200 / FUN_06ef13d0).
struct FStreamingSound
{
    char* Path;                      // +0x00
    uint32_t PathLen;           // +0x04
    uint32_t PathCap;           // +0x08
    uint32_t PathPad;           // +0x0C
    IDAComponent* FileSystem;        // +0x10
    uint32_t FileHandle;        // +0x14
    uint32_t MapHandle;         // +0x18
    unsigned char* MappedView;       // +0x1C
    uint32_t MappedSize;        // +0x20
    uint32_t ReadCursor;        // +0x24
    HACMSTREAM AcmStream;            // +0x28
    ACMSTREAMHEADER AcmHeader;       // +0x2C size 0x54 → +0x80
    uint32_t State;             // +0x80
    unsigned char bBufferReady;      // +0x84
    unsigned char bLoop;             // +0x85
    unsigned char bWantPlay;         // +0x86
    unsigned char bOwnedMap;         // +0x87 retail owned-map flag; port uses FS view
    int32_t Volume;                     // +0x88
    int32_t Pan;                        // +0x8C
    uint32_t FactSamples;       // +0x90
    uint32_t DecodeOutSize;     // +0x94
    uint32_t SourceBlock;       // +0x98
    unsigned char* DataPtr;          // +0x9C
    uint32_t DataSize;          // +0xA0
    uint32_t DataCursor;        // +0xA4
    unsigned char* DecodeHeap;       // +0xA8
    uint32_t DecodeCap;         // +0xAC
    uint32_t DecodeRead;        // +0xB0
    uint32_t DecodeFilled;      // +0xB4
    IDirectSoundBuffer* Buffer;      // +0xB8
    uint32_t BufferBytes;       // +0xBC
    uint32_t WriteCursor;       // +0xC0
    float Threshold;                 // +0xC4
    uint32_t EmittedPcm;        // +0xC8
    uint32_t TrimSamples;       // +0xCC
    int32_t EncoderDelayBytes;          // +0xD0
    unsigned char* FmtBlob;          // +0xD4
    unsigned short PcmTag;           // +0xD8
    unsigned short PcmChannels;      // +0xDA
    uint32_t PcmRate;           // +0xDC
    uint32_t PcmAvg;            // +0xE0
    unsigned short PcmBlock;         // +0xE4
    unsigned short PcmBits;          // +0xE6
    unsigned short PcmCbSize;        // +0xE8
    unsigned short FmtPad;           // +0xEA
    uint32_t AlignedSize;       // +0xEC
    uint32_t BytesPerSec;       // +0xF0
    uint32_t EndMarkF4;         // +0xF4 drain start (play cursor at EOF)
    uint32_t EndMarkF8;         // +0xF8 drain end (write cursor); -1 = not armed
    uint32_t SilenceBytes;      // +0xFC FUN_06ef1ee0 remaining silence
    void* LockPtr1;                  // +0x100
    DWORD LockBytes1;        // +0x104
    void* LockPtr2;                  // +0x108
    DWORD LockBytes2;        // +0x10C
};

class FSoundStreamer final : public IStreamer2, public IAggregateComponent
{
public:
    FSoundStreamer();
    ~FSoundStreamer();

    GENRESULT DACOM_API QueryInterface(const char* Iid, void** Out) override;
    uint32_t DACOM_API AddRef() override;
    uint32_t DACOM_API Release() override;

    int DACOM_API InitializeDevice(FStreamerInitDesc* Desc) override;
    void* DACOM_API CreateStream(const char* Path, IDAComponent* FileSystem,
                                 uint32_t Flags) override;
    GENRESULT DACOM_API DestroyStream(void* Handle) override;
    GENRESULT DACOM_API StopStream(void* Handle) override;
    GENRESULT DACOM_API StartStream(void* Handle) override;
    GENRESULT DACOM_API SetVolume(void* Handle, int32_t Volume) override;
    GENRESULT DACOM_API GetVolume(void* Handle, int32_t* Out) override;
    uint32_t DACOM_API QueryStatus(void* Handle) override;
    uint32_t DACOM_API GetReserved58() override;
    uint32_t DACOM_API GetReserved5C() override;
    GENRESULT DACOM_API SetPan(void* Handle, int32_t Pan) override;
    GENRESULT DACOM_API GetPan(void* Handle, int32_t* Out) override;

    GENRESULT DACOM_API Initialize() override;

    void BindOuter(FStreamerAggDesc* Desc);
    void ShutdownDevice();
    uint32_t PumpStreams();
    void PumpOneStream(FStreamingSound* Stream);
    void MovePendingActive();
    bool OpenOwnDirectSound();
    bool LoadWav(FStreamingSound* Stream, const char* Path, IDAComponent* FileSystem); // FUN_06ef1560
    bool OpenAcmStream(FStreamingSound* Stream);                                      // FUN_06ef13d0
    void FillStreamBlock(FStreamingSound* Stream, uint32_t Want);                // FUN_06ef27a0
    void ReleaseStreamResources(FStreamingSound* Stream); // FUN_06ef1120 + 3910 / 3ea0 / 3ec0

    IDirectSound* DirectSound;    // +0x08
    HWND Window;                  // +0x0C
    uint32_t Extra;          // +0x10
    uint32_t Pad14;
    float Gain;                   // +0x18
    unsigned char bCoInitialized; // +0x1C
    unsigned char Pad1D[3];
    CRITICAL_SECTION Lock;        // +0x20
    uint32_t ActiveCookie;   // +0x38 VC6 vector cookie
    FStreamingSound** ActiveBegin;// +0x3C
    FStreamingSound** ActiveEnd;  // +0x40
    FStreamingSound** ActiveCap;  // +0x44
    uint32_t PendingCookie;  // +0x48
    FStreamingSound** PendingBegin;
    FStreamingSound** PendingEnd;
    FStreamingSound** PendingCap;
    uint32_t Reserved58;     // +0x58
    uint32_t Reserved5C;     // +0x5C
    HANDLE WorkerThread;          // +0x60
    uint32_t ThreadFlags;    // +0x64 bit0 started, bit1 exited, bit7 stop
    HANDLE WakeEvent;             // +0x68
    IDAComponent* Controlling;    // +0x6C
    FStreamerInner Inner;         // +0x70
};

static_assert(sizeof(FSoundStreamerFactory) == 0x0C, "factory is 0xC");
static_assert(offsetof(FSoundStreamer, DirectSound) == 0x08, "DS +0x08");
static_assert(offsetof(FSoundStreamer, Window) == 0x0C, "HWND +0x0C");
static_assert(offsetof(FSoundStreamer, Gain) == 0x18, "Gain +0x18");
static_assert(offsetof(FSoundStreamer, bCoInitialized) == 0x1C, "CoInit +0x1C");
static_assert(offsetof(FSoundStreamer, Lock) == 0x20, "CS +0x20");
static_assert(offsetof(FSoundStreamer, ActiveCookie) == 0x38, "active cookie +0x38");
static_assert(offsetof(FSoundStreamer, Reserved58) == 0x58, "reserved +0x58");
static_assert(offsetof(FSoundStreamer, WorkerThread) == 0x60, "thread +0x60");
static_assert(offsetof(FSoundStreamer, ThreadFlags) == 0x64, "flags +0x64");
static_assert(offsetof(FSoundStreamer, Controlling) == 0x6C, "controlling +0x6C");
static_assert(offsetof(FSoundStreamer, Inner) == 0x70, "inner +0x70");
static_assert(sizeof(FStreamerInner) == 0x0C, "inner is 0x0C");
static_assert(sizeof(FSoundStreamer) == 0x7C, "object is 0x7C");
static_assert(sizeof(ACMSTREAMHEADER) == 0x54, "ACMSTREAMHEADER is 0x54");
static_assert(sizeof(FStreamingSound) == 0x110, "StreamingSound is 0x110");
static_assert(offsetof(FStreamingSound, FileSystem) == 0x10, "FS +0x10");
static_assert(offsetof(FStreamingSound, AcmStream) == 0x28, "HACMSTREAM +0x28");
static_assert(offsetof(FStreamingSound, AcmHeader) == 0x2C, "ACM header +0x2C");
static_assert(offsetof(FStreamingSound, State) == 0x80, "state +0x80");
static_assert(offsetof(FStreamingSound, DataPtr) == 0x9C, "data +0x9C");
static_assert(offsetof(FStreamingSound, DecodeHeap) == 0xA8, "decode heap +0xA8");
static_assert(offsetof(FStreamingSound, Buffer) == 0xB8, "DS buffer +0xB8");
static_assert(offsetof(FStreamingSound, TrimSamples) == 0xCC, "trim +0xCC");
static_assert(offsetof(FStreamingSound, EncoderDelayBytes) == 0xD0, "delay +0xD0");
static_assert(offsetof(FStreamingSound, FmtBlob) == 0xD4, "fmt +0xD4");
static_assert(offsetof(FStreamingSound, PcmTag) == 0xD8, "pcm tag +0xD8");
static_assert(offsetof(FStreamingSound, AlignedSize) == 0xEC, "aligned +0xEC");
static_assert(offsetof(FStreamingSound, BytesPerSec) == 0xF0, "bytes/sec +0xF0");
static_assert(offsetof(FStreamingSound, EndMarkF4) == 0xF4, "drain start +0xF4");
static_assert(offsetof(FStreamingSound, EndMarkF8) == 0xF8, "drain end +0xF8");
static_assert(offsetof(FStreamingSound, SilenceBytes) == 0xFC, "silence +0xFC");

// Freelancer Decomp: a reimplementation of the Freelancer (2003) game client and engine,
// faithful to the behaviour of the original game.
// This file was reconstructed by reverse engineering the binaries of Freelancer 1.1,
// developed by Digital Anvil.
// Author: Giperion.

#pragma once

#include "MaterialLibrary.h"
#include "ShadingInternal.h"

#include <list>
#include <map>
#include <memory>

struct FBatchRecord
{
    uint32_t InternalFlags; // +0x00
    FMaterialSubmitContext Context; // +0x04, borrowed views into this queued record
    float MatrixDataA[12];       // +0x28
    float MatrixDataB[12];       // +0x58
    float Combined[12];          // +0x88
    uint32_t PrimType;                // +0xB8
    uint32_t Start;         // +0xBC
    uint32_t Count;         // +0xC0
    uint32_t Zero;          // +0xC4
    uint32_t Field;         // +0xC8
    FVbListing VertexBufferListing; // +0xCC, formerly OptionalAData
    FIbScratch IndexBufferScratch;  // +0xD8, formerly OptionalBData
    uint32_t Flags;         // +0xE0
    void* Field78;               // +0xE4
    void* Field7C;               // +0xE8
    void* Unused;                // +0xEC (Arg8; sort depth when flags&0x20)

    FBatchRecord();
};

static_assert(sizeof(FBatchRecord) == 0xF0, "retail batch payload size 0xF0");
static_assert(offsetof(FBatchRecord, Context) == 4, "queued context starts at +4");
static_assert(sizeof(FMaterialSubmitContext) == 0x24, "queued context is nine words");
static_assert(offsetof(FBatchRecord, Context) + offsetof(FMaterialSubmitContext, VbListing) == 0x18, "queued VB view +0x18");
static_assert(offsetof(FBatchRecord, Context) + offsetof(FMaterialSubmitContext, IbScratch) == 0x1C, "queued IB view +0x1C");
static_assert(offsetof(FBatchRecord, MatrixDataA) == 0x28, "world matrix follows context");
static_assert(offsetof(FBatchRecord, MatrixDataB) == 0x58, "view matrix +0x58");
static_assert(offsetof(FBatchRecord, Combined) == 0x88, "combined matrix +0x88");
static_assert(sizeof(FVbListing) == 12 && sizeof(FIbScratch) == 8, "queued listing/scratch copy sizes");
static_assert(offsetof(FBatchRecord, VertexBufferListing) == 0xCC, "owned VB listing +0xCC");
static_assert(offsetof(FBatchRecord, IndexBufferScratch) == 0xD8, "owned IB scratch +0xD8");
static_assert(offsetof(FBatchRecord, Flags) == 0xE0, "flags follow optional snapshots");

struct FBatchQueuedRecord
{
    FBatchRecord Retail;
    std::vector<unsigned short> OwnedIndices;
};

struct FBatchBin
{
    int32_t Pad0;
    uint32_t SortKind; // +4
    std::map<IMaterial*, std::list<FBatchQueuedRecord>> Queue;
    uint32_t RecordCount;

    FBatchBin();
};


class FMaterialBatcher;
struct FBatchDispatch;

class FMaterialBatcherController final : public IDAComponent
{
public:
    explicit FMaterialBatcherController(FMaterialBatcher* InOwner);

    GENRESULT DACOM_API QueryInterface(const char* Iid, void** Out) override;
    uint32_t DACOM_API AddRef() override;
    uint32_t DACOM_API Release() override;

    FMaterialBatcher* Owner;
};

class FMaterialBatcher final : public IMaterialBatcher2
{
public:
    FMaterialBatcher();
    ~FMaterialBatcher();

    GENRESULT DACOM_API QueryInterface(const char* Iid, void** Out) override;
    uint32_t DACOM_API AddRef() override;
    uint32_t DACOM_API Release() override;

    GENRESULT DACOM_API ClearRules() override;
    GENRESULT DACOM_API AddRule(uint32_t Kind, const char* Pattern) override;
    GENRESULT DACOM_API RemoveRule(uint32_t Kind, const char* Pattern) override;
    GENRESULT DACOM_API SelectBin(uint32_t Index) override;
    GENRESULT DACOM_API SetBinSortKind(uint32_t Index, void* Value) override;
    GENRESULT DACOM_API SetBinOrder(uint32_t Index, int32_t Key) override;
    GENRESULT DACOM_API FlushBin(uint32_t Index, unsigned char Clear) override;
    GENRESULT DACOM_API FlushAll(unsigned char Clear) override;
    GENRESULT DACOM_API Submit(IMaterial* Material, void* Context, uint32_t PrimType,
                               uint32_t Start, uint32_t Count,
                               uint32_t Zero, uint32_t Field,
                               void* Unused, uint32_t Flags) override;
    GENRESULT DACOM_API ClearBins() override;
    GENRESULT DACOM_API NoOp(void* Arg) override;
    GENRESULT DACOM_API SetEvents(void* NewValue) override;
    GENRESULT DACOM_API GetEvents(void* Arg) override;
    GENRESULT DACOM_API SetRecordTag(void* Value) override;
    GENRESULT DACOM_API GetRecordTag(void* Arg) override;
    GENRESULT DACOM_API SetRecordTagContext(void* Value) override;
    GENRESULT DACOM_API GetRecordTagContext(void* Arg) override;

    FMaterialLibrary Library; // +4  QI 1.11_IMaterialLibrary
    std::vector<FBatchRule> RuleList; // retail this+0x58/+0x5C, slots 3/4/5
    uint32_t RefCount;
    IDAComponent* Outer;
    IRenderPipeline* Pipeline;
    IRPVertexBuffer* RpVertexBuffer;
    IRPIndexBuffer* RpIndexBuffer;
    unsigned char* ScratchBuffer; // Owned byte arena for copied streams.
    uint32_t BufferSize;      // retail owner +0x84 capacity
    uint32_t Bin1SortOrder;
    std::unique_ptr<FBatchBin> Bins[2];
    uint32_t BinOrder[2];     // retail this+0x60/+0x64
    FBatchBin* SelectedBin;        // retail this+0x70, slots 6/7
    IMaterialBatcherEvents* CachedInterface; // retail this+0x74, slots 14/15 (AddRef/Release-managed)
    void* Field78;                 // retail this+0x78, slots 16/17 (plain)
    void* Field7C;                 // retail this+0x7C, slots 18/19 (plain)
    uint32_t FrameValue;      // retail this+0x88
    uint32_t PendingFrameValue; // retail this+0x8C
    FMaterialBatcherController Controller; // retail nondelegating unknown @ +0x94
    IDAComponent* ControllingUnknown; // retail +90, distinct from initialized Outer+50

    GENRESULT InitializeFromDescriptor(const FMaterialAggregateDesc* Desc);
    GENRESULT StartupFromOuter();
    void ResetScratch();
    void InitializeScratch(uint32_t Size);
    void* AllocateScratch(uint32_t Size);
    unsigned short* CloneIndices(const unsigned short* Source, uint32_t Count);
    void CloneVertexStream(const FRpVertexStream& Source, FRpVertexStream& Dest, uint32_t IndexCount);
    void CopyVertexSource(const FRpVertexSrcDesc& Source, FRpVertexSrcDesc& Dest);
    FRpVertexSrcDesc* CloneVertexSource(const FRpVertexSrcDesc& Source);
    void ParseBatchedMaterials();
    void FlushOne(FBatchBin* Bin, unsigned char Clear);
    void ProcessQueuedBin(FBatchBin* Bin);
    void BuildBatchDispatch(FBatchBin* Bin, FBatchDispatch* Dispatch);
    void ClearBin(FBatchBin* Bin);
};

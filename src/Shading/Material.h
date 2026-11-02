// Freelancer Decomp: a reimplementation of the Freelancer (2003) game client and engine,
// faithful to the behaviour of the original game.
// This file was reconstructed by reverse engineering the binaries of Freelancer 1.1,
// developed by Digital Anvil.
// Author: Giperion.

#pragma once

#include "ShadingInternal.h"

class FMaterial;

// Property accessors return records from these shared contiguous schemas.
FShadingPropertyRecord* GetBtDetailPropertySchema();
FShadingPropertyRecord* GetDetailMapPropertySchema();
FShadingPropertyRecord* GetAtmospherePropertySchema();
FShadingPropertyRecord* GetSinglePassPropertySchema();
FShadingPropertyRecord* GetIllumDetailPropertySchema();
FShadingPropertyRecord* GetEcEtPropertySchema();
FShadingPropertyRecord* GetDcDtEtPropertySchema();

class FMaterialInner final : public IDAComponent
{
public:
    FMaterialInner();

    GENRESULT DACOM_API QueryInterface(const char* Iid, void** Out) override;
    uint32_t DACOM_API AddRef() override;
    uint32_t DACOM_API Release() override;

    FMaterial* OwnerFromThis();
};

class FMaterial final : public IMaterial
{
public:
    FMaterial(const char* FactoryName, const char* ClassName, EMaterialBind BindKind,
              EMaterialSubmit SubmitKind);
    ~FMaterial() noexcept(false);

    GENRESULT DACOM_API QueryInterface(const char* Iid, void** Out) override;
    uint32_t DACOM_API AddRef() override;
    uint32_t DACOM_API Release() override;

    GENRESULT DACOM_API Initialize(IDAComponent* Outer) override;
    GENRESULT DACOM_API LoadFromFileSystem(void* FileSystem) override;
    GENRESULT DACOM_API SaveProperties(void* FileSystem) override;
    GENRESULT DACOM_API ApplyVerify(void* Context, void* Extra) override;
    GENRESULT DACOM_API ResolveTextures(float Delta) override;
    GENRESULT DACOM_API Bind(void* Context) override;
    GENRESULT DACOM_API Teardown(void* Context) override;
    GENRESULT DACOM_API Submit(void* Context, uint32_t PrimType, uint32_t Start,
                               uint32_t Count, uint32_t Zero, uint32_t Field,
                               uint32_t Flags) override;
    GENRESULT DACOM_API SetName(const char* Name) override;
    GENRESULT DACOM_API GetName(char* Out, uint32_t Size) override;
    GENRESULT DACOM_API CopyClassName(char* Out, uint32_t Size) override;
    GENRESULT DACOM_API GetPassCount(int* Out) override;
    GENRESULT DACOM_API GetRenderFlags(uint32_t* Out) override;
    GENRESULT DACOM_API GetParameterCount(uint32_t* OutCount) override;
    GENRESULT DACOM_API GetParameterDescriptor(uint32_t Index, void* OutDesc) override;
    GENRESULT DACOM_API GetParameterValue(uint32_t Index, void* InOutDesc) override;
    GENRESULT DACOM_API SetParameterValue(uint32_t Index, void* Value) override;
    const char* __thiscall ClassName() override;
    void* __thiscall ScalarDeletingDestructor(unsigned char bDelete) override;
    GENRESULT __thiscall InitializeFromDescriptor(void* Arg) override;
    GENRESULT __thiscall ApplyInitialRenderStates(void* Arg) override;
    GENRESULT __thiscall Setup() override;
    bool __thiscall ValueWithinDeclaredRange(uint32_t Index, void* Entry, void* Value) override;
    GENRESULT __thiscall StoreParameterValue(uint32_t Index, void* Entry, void* Value) override;
    uint32_t __thiscall GetParameterTable(void** OutBase) override;
    void* __thiscall GetParameterAddress(uint32_t Index) override;
    GENRESULT DACOM_API InnerQueryInterface(const char* Iid, void** Out) override;
    uint32_t DACOM_API InnerAddRef() override;
    uint32_t DACOM_API InnerRelease() override;
    void ConfigureAggregation(FMaterialAggregateDesc* Desc);
    IRenderPipeline* Pipeline; // +4
    ITextureLibrary2* TexLib;    // Borrowed from the outer component.
    IVertexBufferManager* VBMgr;
    IRPDraw* RPDraw;
    const char* InstanceName;    // +14  LAB_06ebd810 GetCRC32 → IMatAnimLibrary slot 5
    FTexBlock* TexBlocks;        // +18
    uint32_t TexCount;      // +1C
    float DiffuseR;              // +20
    float DiffuseG;
    float DiffuseB;
    float Opacity;               // +2C
    float Ambient[4];            // +30
    float Specular[4];           // +40
    float Emissive[4];           // +50
    float SpecularPower;         // +60
    unsigned char bAlphablend;   // +64  "Oc"
    unsigned char bAlphatest;    // +65
    unsigned char Pad66[2];
    IImaInstance* MatAnim;               // +68  DcDt IMAInstance; Atmosphere retail = FadeOff float
    uint32_t Pad6C;         // +6C  Atmosphere retail = Alpha float
    FMaterialInner Inner;        // +70  SinglePass inner QI; Atmosphere retail = Scale float
    unsigned char bAtmosphereReady; // +74  byte FUN_06eb2b10 / FUN_06eb2b40 / LAB_06eb2d40
    unsigned char Pad75[3];
    float FadeOff;               // Atmosphere property 6, default 1.0 (DAT_06ed11c4)
    float Alpha;                 // Atmosphere property 7, default 0.75 (DAT_06ed11c8)
    float Scale;                 // Atmosphere property 8, default DAT_06ed11cc ≈ 1.025
    uint32_t DetailFlipU;
    uint32_t DetailFlipV;
    float DetailTileRate0;
    float DetailTileRate1;
    uint32_t DetailPassCount; // Detail +0x74, illum/masked +0x78: 1 or 2 passes
    bool bDetailSecondStageVerified; // Detail +0x79, illum/masked +0x7D
    bool bDetailMultiPassEnabled; // Detail +0x78, illum/masked +0x7C: global record0 gate


    uint32_t RefCount;
    const char* FactoryName;
    const char* ClassNameText;  // slot 20 answers with this, else FactoryName
    EMaterialBind BindKind;
    EMaterialSubmit SubmitKind;
    IDAComponent* Outer;
    bool bSecondaryTextureEnabled;
    bool bSecondaryTextureVerified;
    uint32_t TextureAllocationCount;


    void SetInstanceName(const char* Name);
    GENRESULT InitializeOwnedSchema();
    GENRESULT LoadDeclaredSchema(IFileSystem* FileSystem);
    GENRESULT ApplyVerifyMatAnim();
    void ApplyOcFlag();
    void BindDcDtTables(void* Context);
    void BindLightMapTables(void* Context);
    void BindNebulaTables(void* Context);
    void BindAtmosphereTables();
    void BindDcDtEtTables(void* Context);
    void BindEcEtTables(void* Context);
    void BindDetailMapTables(void* Context);
    GENRESULT ApplyVerifyDetailFamily();
    GENRESULT ApplyVerifyIllumDetail();
    bool ApplyDetailPassState(uint32_t Pass);
    bool ApplyIllumDetailPassState(uint32_t Pass);
    bool ApplyMaskedDetailPassState(uint32_t Pass);
    void BindBtDetailTables(void* Context);
    GENRESULT SubmitIndexedListing(FMaterialSubmitContext* Context, uint32_t PrimType,
                                   uint32_t Start, uint32_t Count, uint32_t IbStart,
                                   uint32_t IndexSpan);
    GENRESULT SubmitCopiedVertices(FMaterialSubmitContext* Context, uint32_t PrimType,
                                   uint32_t Start, uint32_t Count, uint32_t IbStart,
                                   uint32_t IndexSpan);
    GENRESULT SubmitAlphaTest(void* Context, uint32_t PrimType, uint32_t Start,
                              uint32_t Count, uint32_t IbStart, uint32_t IndexSpan,
                              uint32_t Flags);
    GENRESULT SubmitKernel(void* Context, uint32_t PrimType, uint32_t Start, uint32_t Count,
                           uint32_t Zero, uint32_t Field, uint32_t Flags);
    GENRESULT SubmitDcDtTwoSecondPass(void* Context, uint32_t PrimType, uint32_t Start,
                                 uint32_t Count, uint32_t Zero, uint32_t Field);
    GENRESULT SubmitBtDetailTwoSecondPass(void* Context, uint32_t PrimType,
                                          uint32_t Start, uint32_t Count,
                                          uint32_t IbStart, uint32_t IndexSpan);
    GENRESULT SubmitAtmosphereLock(void* Context, uint32_t PrimType, uint32_t Start,
                                   uint32_t Count, uint32_t Zero, uint32_t Field);
};

class FBatchedFace final : public IBatchedMaterial
{
public:
    FBatchedFace();

    GENRESULT DACOM_API QueryInterface(const char* Iid, void** Out) override;
    uint32_t DACOM_API AddRef() override;
    uint32_t DACOM_API Release() override;
    GENRESULT DACOM_API CreateInnerMaterial(void* Name) override;
    GENRESULT DACOM_API SetBatchBin(void* Value) override;
    GENRESULT DACOM_API GetBatchBin(void** Out) override;
    GENRESULT DACOM_API Bind(void* Context) override;
    GENRESULT DACOM_API Teardown(void* Context) override;
    GENRESULT DACOM_API Submit(void* Context, uint32_t PrimType, uint32_t Start, uint32_t Count,
                              uint32_t Zero, uint32_t Field, uint32_t Flags) override;

    class FBatchedMaterial* OwnerFromThis();
};

class FBatchedController final : public IDAComponent
{
public:
    GENRESULT DACOM_API QueryInterface(const char* Iid, void** Out) override;
    uint32_t DACOM_API AddRef() override;
    uint32_t DACOM_API Release() override;
    class FBatchedMaterial* OwnerFromThis();
};

class FBatchedMaterial final : public IMaterial
{
public:
    FBatchedMaterial();
    ~FBatchedMaterial() noexcept(false);

    GENRESULT DACOM_API QueryInterface(const char* Iid, void** Out) override;
    uint32_t DACOM_API AddRef() override;
    uint32_t DACOM_API Release() override;

    GENRESULT DACOM_API Initialize(IDAComponent* Outer) override;
    GENRESULT DACOM_API LoadFromFileSystem(void* FileSystem) override;
    GENRESULT DACOM_API SaveProperties(void* FileSystem) override;
    GENRESULT DACOM_API ApplyVerify(void* Context, void* Extra) override;
    GENRESULT DACOM_API ResolveTextures(float Delta) override;
    GENRESULT DACOM_API Bind(void* Context) override;
    GENRESULT DACOM_API Teardown(void* Context) override;
    GENRESULT DACOM_API Submit(void* Context, uint32_t PrimType, uint32_t Start,
                               uint32_t Count, uint32_t Zero, uint32_t Field,
                               uint32_t Flags) override;
    GENRESULT DACOM_API SetName(const char* Name) override;
    GENRESULT DACOM_API GetName(char* Out, uint32_t Size) override;
    GENRESULT DACOM_API CopyClassName(char* Out, uint32_t Size) override;
    GENRESULT DACOM_API GetPassCount(int* Out) override;
    GENRESULT DACOM_API GetRenderFlags(uint32_t* Out) override;
    GENRESULT DACOM_API GetParameterCount(uint32_t* OutCount) override;
    GENRESULT DACOM_API GetParameterDescriptor(uint32_t Index, void* OutDesc) override;
    GENRESULT DACOM_API GetParameterValue(uint32_t Index, void* InOutDesc) override;
    GENRESULT DACOM_API SetParameterValue(uint32_t Index, void* Value) override;
    const char* __thiscall ClassName() override;
    void* __thiscall ScalarDeletingDestructor(unsigned char bDelete) override;
    GENRESULT __thiscall InitializeFromDescriptor(void* Arg) override;
    GENRESULT __thiscall ApplyInitialRenderStates(void* Arg) override;
    GENRESULT __thiscall Setup() override;
    bool __thiscall ValueWithinDeclaredRange(uint32_t Index, void* Entry, void* Value) override;
    GENRESULT __thiscall StoreParameterValue(uint32_t Index, void* Entry, void* Value) override;
    uint32_t __thiscall GetParameterTable(void** OutBase) override;
    void* __thiscall GetParameterAddress(uint32_t Index) override;
    GENRESULT DACOM_API InnerQueryInterface(const char* Iid, void** Out) override;
    uint32_t DACOM_API InnerAddRef() override;
    uint32_t DACOM_API InnerRelease() override;
    void ConfigureAggregation(FMaterialAggregateDesc* Desc);
    GENRESULT InitializeFromDescriptor(FMaterialAggregateDesc* Desc);
    FBatchedFace Batched; // +4  QI 1.11_IBatchedMaterial (vptr only)
    uint32_t Field8; // +8  LAB_06eb4480 / FUN_06eb40f0 default -1
    IDAComponent* InnerController; // +C retained nondelegating factory controller
    IMaterial* Inner;     // +10 LAB_06eb4380
    void* Batcher;        // +14 Initialize QI IMaterialBatcher
    uint32_t RefCount;
    IDAComponent* Outer;

    FBatchedController Controller;
    IMaterial* InnerMaterial() const;
};

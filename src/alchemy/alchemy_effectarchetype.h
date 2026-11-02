// Freelancer Decomp: a reimplementation of the Freelancer (2003) game client and engine,
// faithful to the behaviour of the original game.
// This file was reconstructed by reverse engineering the binaries of Freelancer 1.1,
// developed by Digital Anvil.
// Author: Giperion.

#pragma once

#include "alchemy_types.h"

#include <cstring>
#include <string>

// alchemy.dll — DACOM particle FX scaffolding.
// FFxEffectArchetype: the IFxEffectArchetype component (rec16/rec8 tables).
namespace AlchemyImpl
{
    class FFxEffectArchetype final : public IFxEffectArchetype
    {
    public:
        FFxEffectArchetype()
            : RefCount_(1), NextId_(0), Rec16_(nullptr), Rec16Count_(0), Rec16Capacity_(0),
              Rec8_(nullptr), Rec8Count_(0), Rec8Capacity_(0)
        {
            std::memset(Header16_, 0, sizeof(Header16_));
        }

        ~FFxEffectArchetype()
        {
            delete[] Rec8_;
            Rec8_ = nullptr;
            Rec8Count_ = 0;
            Rec8Capacity_ = 0;
            delete[] Rec16_;
            Rec16_ = nullptr;
            Rec16Count_ = 0;
            Rec16Capacity_ = 0;
        }

        GENRESULT DACOM_API QueryInterface(const char* Iid, void** Out) override;
        uint32_t DACOM_API AddRef() override { return ++RefCount_; }
        uint32_t DACOM_API Release() override;

        GENRESULT DACOM_API ResetRecords() override;
        GENRESULT DACOM_API SetName(const char* Name) override;
        const char* DACOM_API GetName() override { return Name_.c_str(); }

        GENRESULT DACOM_API AddRec16(const FAleRec16* Rec, uint32_t* OutId,
                                     uint32_t Flag) override;
        GENRESULT DACOM_API RemoveRec16(uint32_t Id) override;
        GENRESULT DACOM_API Slot8(uint32_t Id, uint32_t Value) override;
        GENRESULT DACOM_API AddRec8(const FAleRec8* Rec) override;
        GENRESULT DACOM_API RemoveRec8(const FAleRec8* Rec) override;

        uint32_t DACOM_API GetRec16Count() override { return Rec16Count_; }
        uint32_t DACOM_API GetRec8Count() override { return Rec8Count_; }

        GENRESULT DACOM_API GetRec16(uint32_t Index, FAleRec16* Out) override;
        GENRESULT DACOM_API GetRec8(uint32_t Index, FAleRec8* Out) override;
        GENRESULT DACOM_API GetRec16ById(uint32_t Id, FAleRec16* Out) override;
        GENRESULT DACOM_API FindRec8Index(const FAleRec8* Rec, uint32_t* OutIndex) override;
        GENRESULT DACOM_API RemapRec16FieldD(uint32_t OldId, uint32_t NewId) override;
        GENRESULT DACOM_API GetHeader16(unsigned char* Out16) override;
        GENRESULT DACOM_API SetHeader16(const unsigned char* Header16) override;

        GENRESULT DACOM_API Slot20(void* /*Arg*/) override { return ResetRecords(); }

    private:
        void AppendRec16(const FAleRec16& Record);
        void AppendRec8(const FAleRec8& Record);

        uint32_t RefCount_;
        std::string Name_;
        unsigned char Header16_[16];
        uint32_t NextId_;
        FAleRec16* Rec16_;
        uint32_t Rec16Count_;
        uint32_t Rec16Capacity_;
        FAleRec8* Rec8_;
        uint32_t Rec8Count_;
        uint32_t Rec8Capacity_;
    };

} // namespace AlchemyImpl

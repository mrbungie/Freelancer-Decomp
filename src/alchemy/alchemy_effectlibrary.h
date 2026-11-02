// Freelancer Decomp: a reimplementation of the Freelancer (2003) game client and engine,
// faithful to the behaviour of the original game.
// This file was reconstructed by reverse engineering the binaries of Freelancer 1.1,
// developed by Digital Anvil.
// Author: Giperion.

#pragma once

#include "alchemy_types.h"
#include <map>

// alchemy.dll — DACOM particle FX scaffolding.
// FFxEffectLibrary: the IFxEffectLibrary component (archetype registry, ALE
// load/save, effect instantiation).
namespace AlchemyImpl
{
    class FFxEffectLibrary final : public IFxEffectLibrary
    {
    public:
        FFxEffectLibrary()
            : RefCount_(1), Entries_(new FArchetypeMap), MapOwner_(this, Entries_),
              Head_(nullptr), Tail_(nullptr), EntryCount_(0)
        {
        }

        ~FFxEffectLibrary() noexcept(false);

        GENRESULT DACOM_API QueryInterface(const char* Iid, void** Out) override;
        uint32_t DACOM_API AddRef() override { return ++RefCount_; }
        uint32_t DACOM_API Release() override;

        GENRESULT DACOM_API BindAlchemy(IAlchemy* Alchemy, IFxNodeLibrary* NodeLibrary) override;
        GENRESULT DACOM_API InsertArchetype(IFxEffectArchetype* Archetype,
                                            uint32_t* OutHash) override;
        GENRESULT DACOM_API RemoveByHash(uint32_t Hash) override;
        GENRESULT DACOM_API CreateEffect(void* /*Unused*/, void* Key, IFxEffect** Out) override;
        GENRESULT DACOM_API SaveAle(IFileSystem* Filesys) override;
        GENRESULT DACOM_API LoadAle(IFileSystem* Filesys) override;
        GENRESULT DACOM_API ClearEffects() override;
        GENRESULT DACOM_API EnumerateArchetypes(FEffectLibraryEnumCallback Callback, void* Context) override;
        GENRESULT DACOM_API FindByHash(uint32_t Hash, IFxEffectArchetype** Out) override;
        GENRESULT DACOM_API MakeUniqueName(const char* Name, const char** Out) override;
        GENRESULT DACOM_API RemapAllRec16FieldD(uint32_t OldId, uint32_t NewId) override;
        GENRESULT DACOM_API Slot14(void* /*Unused*/) override { return ClearEffects(); }

    private:
        struct FAleLibEntry
        {
            struct FArchetypeReference
            {
                FArchetypeReference() : Archetype(nullptr) {}
                explicit FArchetypeReference(IFxEffectArchetype* Value) : Archetype(Value)
                {
                    if (Archetype != nullptr)
                        Archetype->AddRef();
                }
                FArchetypeReference(const FArchetypeReference& Other)
                    : FArchetypeReference(Other.Archetype) {}
                ~FArchetypeReference() { Reset(); }
                FArchetypeReference& operator=(const FArchetypeReference&) = delete;
                void Reset()
                {
                    if (Archetype != nullptr)
                    {
                        Archetype->Release();
                        Archetype = nullptr;
                    }
                }
                IFxEffectArchetype* Archetype;
            };

            explicit FAleLibEntry(const FArchetypeReference& Value)
                : Next(nullptr), Prev(nullptr), Reference(Value) {}
            FAleLibEntry* Next;
            FAleLibEntry* Prev;
            FArchetypeReference Reference;
        };

        using FArchetypeMap = std::map<uint32_t, FAleLibEntry>;
        struct FMapStorageOwner
        {
            FMapStorageOwner(FFxEffectLibrary* InOwner, FArchetypeMap* InStorage)
                : Owner(InOwner), Storage(InStorage) {}
            ~FMapStorageOwner() noexcept(false);
            FMapStorageOwner(const FMapStorageOwner&) = delete;
            FMapStorageOwner& operator=(const FMapStorageOwner&) = delete;

            FFxEffectLibrary* Owner;
            FArchetypeMap* Storage;
        };

        unsigned char ContainsHash(uint32_t Hash) const;

        void ClearEffectList();
        void DetachEffectList();
        void ClearMapEntries(FArchetypeMap* Storage);
        void DestroyMapStorage(FArchetypeMap* Storage);

        uint32_t RefCount_;
        FArchetypeMap* Entries_;
        FMapStorageOwner MapOwner_;
        FAleLibEntry* Head_;
        FAleLibEntry* Tail_;
        uint32_t EntryCount_;
    };

} // namespace AlchemyImpl

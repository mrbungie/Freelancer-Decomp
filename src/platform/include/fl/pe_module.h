// Read-only access to the resources of a Windows PE file (.dll/.exe), on any host.
// Freelancer keeps its strings, infocards, dialogs and ids tables in resource DLLs
// (Resources.dll, InfoCards.dll, ...). This is the host-independent replacement for
// LoadLibrary + FindResource + LoadResource + LoadString on such modules.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace fl
{
    class PeModule
    {
    public:
        // Returns nullptr when `path` is not a readable PE file with a resource directory.
        static PeModule* Open(const char* path);

        // type/name: integer ids (MAKEINTRESOURCE) or names. Pass id = 0 and a non-null *Name for
        // string identifiers. lang 0 = first available language.
        // Returns a pointer into the loaded image (valid until the module is destroyed).
        const uint8_t* Find(uint32_t typeId, const char* typeName, uint32_t nameId, const char* nameStr,
                            uint16_t lang, uint32_t* size) const;

        // RT_STRING helper: copies string `id` as UTF-16 code units; returns its length (0 if missing).
        size_t LoadString16(uint32_t id, uint16_t* out, size_t capacity) const;

        const std::string& Path() const { return path_; }

    private:
        bool Parse();
        const uint8_t* RvaToPtr(uint32_t rva, uint32_t size) const;
        struct Section { uint32_t va, vsize, rawPtr, rawSize; };
        std::string path_;
        std::vector<uint8_t> image_;
        std::vector<Section> sections_;
        uint32_t resRva_ = 0, resSize_ = 0;
    };
}

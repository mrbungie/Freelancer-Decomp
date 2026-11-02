#include "fl/pe_module.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

namespace fl
{
    namespace
    {
        template <class T> bool Read(const std::vector<uint8_t>& b, size_t off, T* out)
        {
            if (off + sizeof(T) > b.size()) return false;
            memcpy(out, b.data() + off, sizeof(T));
            return true;
        }
        uint16_t U16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
        uint32_t U32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
    }

    PeModule* PeModule::Open(const char* path)
    {
        FILE* f = fopen(path, "rb");
        if (!f) return nullptr;
        PeModule* m = new PeModule();
        m->path_ = path;
        fseek(f, 0, SEEK_END);
        long n = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (n <= 0x40) { fclose(f); delete m; return nullptr; }
        m->image_.resize((size_t)n);
        size_t got = fread(m->image_.data(), 1, (size_t)n, f);
        fclose(f);
        if (got != (size_t)n || !m->Parse()) { delete m; return nullptr; }
        return m;
    }

    bool PeModule::Parse()
    {
        const uint8_t* b = image_.data();
        if (image_.size() < 0x40 || b[0] != 'M' || b[1] != 'Z') return false;
        uint32_t pe = U32(b + 0x3c);
        if (pe + 0x18 > image_.size() || memcmp(b + pe, "PE\0\0", 4) != 0) return false;
        uint16_t numSections = U16(b + pe + 6);
        uint16_t optSize = U16(b + pe + 20);
        const uint8_t* opt = b + pe + 24;
        uint16_t magic = U16(opt);
        uint32_t dirOff = magic == 0x20b ? 112 : 96;                // PE32+ vs PE32
        if (pe + 24 + dirOff + 8 * 3 > image_.size()) return false;
        resRva_ = U32(opt + dirOff + 2 * 8);
        resSize_ = U32(opt + dirOff + 2 * 8 + 4);
        const uint8_t* sec = opt + optSize;
        for (uint16_t i = 0; i < numSections; ++i, sec += 40)
        {
            if ((size_t)(sec - b) + 40 > image_.size()) return false;
            sections_.push_back({U32(sec + 12), U32(sec + 8), U32(sec + 20), U32(sec + 16)});
        }
        return resRva_ != 0;
    }

    const uint8_t* PeModule::RvaToPtr(uint32_t rva, uint32_t size) const
    {
        for (const Section& s : sections_)
        {
            uint32_t span = s.rawSize > s.vsize ? s.rawSize : s.vsize;
            if (rva >= s.va && rva < s.va + span)
            {
                size_t off = (size_t)s.rawPtr + (rva - s.va);
                if (off + size <= image_.size()) return image_.data() + off;
                return nullptr;
            }
        }
        return nullptr;
    }

    const uint8_t* PeModule::Find(uint32_t typeId, const char* typeName, uint32_t nameId, const char* nameStr,
                                  uint16_t lang, uint32_t* size) const
    {
        const uint8_t* root = RvaToPtr(resRva_, 16);
        if (!root) return nullptr;

        auto matchEntry = [&](const uint8_t* dir, uint32_t id, const char* name, uint32_t* subOffset, bool* isDir) -> bool {
            uint16_t nNamed = U16(dir + 12), nId = U16(dir + 14);
            const uint8_t* e = dir + 16;
            for (uint32_t i = 0; i < (uint32_t)nNamed + nId; ++i, e += 8)
            {
                uint32_t key = U32(e), val = U32(e + 4);
                bool hit = false;
                if (key & 0x80000000u)               // named entry: UTF-16 string at root + (key & 0x7fffffff)
                {
                    if (name)
                    {
                        const uint8_t* s = root + (key & 0x7fffffffu);
                        uint16_t len = U16(s);
                        hit = strlen(name) == len;
                        for (uint16_t k = 0; hit && k < len; ++k)
                            hit = (U16(s + 2 + 2 * k) < 128) && tolower((int)U16(s + 2 + 2 * k)) == tolower((unsigned char)name[k]);
                    }
                }
                else hit = !name && key == id;
                if (hit) { *isDir = (val & 0x80000000u) != 0; *subOffset = val & 0x7fffffffu; return true; }
            }
            return false;
        };

        uint32_t off; bool isDir;
        if (!matchEntry(root, typeId, typeName, &off, &isDir) || !isDir) return nullptr;
        const uint8_t* nameDir = root + off;
        if (!matchEntry(nameDir, nameId, nameStr, &off, &isDir)) return nullptr;
        const uint8_t* dataEntry;
        if (isDir)
        {
            const uint8_t* langDir = root + off;
            uint32_t total = (uint32_t)U16(langDir + 12) + U16(langDir + 14);
            if (total == 0) return nullptr;
            const uint8_t* e = langDir + 16;
            const uint8_t* chosen = e;
            if (lang != 0)
                for (uint32_t i = 0; i < total; ++i, e += 8)
                    if ((U32(e) & 0x7fffffffu) == lang) { chosen = e; break; }
            dataEntry = root + (U32(chosen + 4) & 0x7fffffffu);
        }
        else dataEntry = root + off;

        uint32_t dataRva = U32(dataEntry), dataSize = U32(dataEntry + 4);
        const uint8_t* p = RvaToPtr(dataRva, dataSize);
        if (p && size) *size = dataSize;
        return p;
    }

    size_t PeModule::LoadString16(uint32_t id, uint16_t* out, size_t capacity) const
    {
        uint32_t size = 0;
        const uint8_t* block = Find(6, nullptr, (id >> 4) + 1, nullptr, 0, &size);
        if (!block) return 0;
        uint32_t pos = 0;
        for (uint32_t i = 0; i < 16 && pos + 2 <= size; ++i)
        {
            uint16_t len = U16(block + pos);
            pos += 2;
            if (i == (id & 15))
            {
                size_t n = len < capacity ? len : (capacity ? capacity - 1 : 0);
                for (size_t k = 0; k < n; ++k) out[k] = U16(block + pos + 2 * k);
                if (capacity) out[n] = 0;
                return n;
            }
            pos += (uint32_t)len * 2;
        }
        return 0;
    }
}

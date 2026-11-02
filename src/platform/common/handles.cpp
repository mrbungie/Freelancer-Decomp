#include <mutex>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr uintptr_t kSmall = 0x10000;
    constexpr uint32_t kBase = 0x40000000;          // far above any plain integer the game passes in pointer slots
    std::mutex G_Mutex;
    std::unordered_map<const void*, uint32_t>& ByPtr() { static std::unordered_map<const void*, uint32_t> m; return m; }
    std::vector<const void*>& ById() { static std::vector<const void*> v; return v; }
}

namespace fl
{
    uint32_t PtrToHandle(const void* p)
    {
        const uintptr_t v = reinterpret_cast<uintptr_t>(p);
        if (v < kSmall) return static_cast<uint32_t>(v);
        if (v >= 0xFFFFFFFFFFFFFFF0ull || (v >= 0xFFFFFFF0u && v <= 0xFFFFFFFFu)) return static_cast<uint32_t>(v);   // -1 sentinels
        std::lock_guard<std::mutex> lock(G_Mutex);
        auto it = ByPtr().find(p);
        if (it != ByPtr().end()) return it->second;
        const uint32_t id = kBase + static_cast<uint32_t>(ById().size()) * 8u;
        ById().push_back(p);
        ByPtr().emplace(p, id);
        return id;
    }

    void* HandleToPtr(uint32_t h)
    {
        if (h < kSmall) return reinterpret_cast<void*>(static_cast<uintptr_t>(h));
        if (h >= 0xFFFFFFF0u) return reinterpret_cast<void*>(static_cast<intptr_t>(static_cast<int32_t>(h)));
        std::lock_guard<std::mutex> lock(G_Mutex);
        if (h < kBase || ((h - kBase) & 7u)) return reinterpret_cast<void*>(static_cast<uintptr_t>(h));   // not ours: leave as a plain integer
        const size_t idx = (h - kBase) / 8u;
        return idx < ById().size() ? const_cast<void*>(ById()[idx]) : nullptr;
    }
}

#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace xray::render::vulkan
{
// Borrowed unordered_map values survive rehashes. Invalidate before replacing
// or destroying the registry. Exact names guard all hash/index collisions.
template <class Value> class ShaderPairCache
{
    struct Entry { std::string vertex, fragment; const Value* value{}; };
    std::array<Entry,128> entries_{};
    static size_t slot(const char* vertex, const char* fragment)
    {
        uint64_t hash=14695981039346656037ull;
        for (const char* name : {vertex,fragment})
        {
            for (const unsigned char* p=reinterpret_cast<const unsigned char*>(name);*p;++p)
                hash=(hash^*p)*1099511628211ull;
            hash=(hash^0xffu)*1099511628211ull;
        }
        return size_t(hash)%128;
    }
public:
    const Value* find(const char* vertex, const char* fragment) const
    {
        const auto& item=entries_[slot(vertex,fragment)];
        return item.value && item.vertex==vertex && item.fragment==fragment ? item.value : nullptr;
    }
    void remember(const char* vertex, const char* fragment, const Value* value)
    {
        if (!value) return;
        auto& item=entries_[slot(vertex,fragment)];
        item.vertex=vertex;item.fragment=fragment;item.value=value;
    }
    void clear() { for(auto& item:entries_)item.value=nullptr; }
};
}

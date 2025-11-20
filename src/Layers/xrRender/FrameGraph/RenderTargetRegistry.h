#pragma once

#include "FGResource.h"
#include "xrCore/xrstring.h"
#include <unordered_map>

namespace xray::render::framegraph {

class RenderTargetRegistry {
public:
    RenderTargetRegistry() = default;
    ~RenderTargetRegistry() = default;

    void RegisterRT(const char* name, VirtualResourceHandle handle) {
        shared_str key = name;

        #ifdef DEBUG
        auto it = m_registry.find(key);
        if (it != m_registry.end()) {
        }
        #endif

        m_registry[key] = handle;
    }

    void RegisterAliases(VirtualResourceHandle handle, const std::initializer_list<const char*>& names) {
        for (const char* name : names) {
            RegisterRT(name, handle);
        }
    }

    VirtualResourceHandle GetRT(const char* name) const {
        shared_str key = name;
        auto it = m_registry.find(key);

        if (it == m_registry.end()) {
            R_ASSERT2(false, make_string("RT not found: %s", name));
        }

        return it->second;
    }

    VirtualResourceHandle TryGetRT(const char* name) const {
        shared_str key = name;
        auto it = m_registry.find(key);

        if (it != m_registry.end()) {
            return it->second;
        }

        return VirtualResourceHandle{};
    }

    bool HasRT(const char* name) const {
        shared_str key = name;
        return m_registry.find(key) != m_registry.end();
    }

    xr_vector<shared_str> GetAllNames() const {
        xr_vector<shared_str> names;
        names.reserve(m_registry.size());
        for (const auto& pair : m_registry) {
            names.push_back(pair.first);
        }
        return names;
    }

    void PrintRegistry() const {
        Msg("! [RTRegistry] ═══ Registered render targets (%u total) ═══", m_registry.size());
        for (const auto& pair : m_registry) {
            Msg("!   '%s' -> Handle[%u]",
                pair.first.c_str(),
                pair.second.index);
        }
        Msg("! [RTRegistry] ═══════════════════════════════════════");
    }

    void Clear() {
        m_registry.clear();
    }

private:
    std::unordered_map<shared_str, VirtualResourceHandle> m_registry;
};

}

#pragma once

#include "FGResource.h"
#include "xrCore/xrstring.h"
#include <unordered_map>

namespace xray::render::framegraph {

// ═══════════════════════════════════════════════════
//  RENDER TARGET REGISTRY
// ═══════════════════════════════════════════════════
//
// Centralized registry for all render targets, enabling lookup by name.
// Supports multiple aliases for the same RT (e.g., "rt_Position", "s_position", "s_pos").
//
// Purpose:
// - Map shader texture names to FrameGraph virtual resource handles
// - Support X-Ray's existing naming conventions
// - Enable shader reflection to find required RTs automatically
//
class RenderTargetRegistry {
public:
    RenderTargetRegistry() = default;
    ~RenderTargetRegistry() = default;

    // ═══════════════════════════════════════════════════
    //  REGISTRATION
    // ═══════════════════════════════════════════════════

    // Register a render target with a name
    void RegisterRT(const char* name, VirtualResourceHandle handle) {
        shared_str key = name;

        // Check for duplicates (debug only)
        #ifdef DEBUG
        auto it = m_registry.find(key);
        if (it != m_registry.end()) {
        }
        #endif

        m_registry[key] = handle;
    }

    // Register multiple aliases for same RT
    // Example: RegisterAliases(fg_rt_Position, {"s_position", "s_pos"})
    void RegisterAliases(VirtualResourceHandle handle, const std::initializer_list<const char*>& names) {
        for (const char* name : names) {
            RegisterRT(name, handle);
        }
    }

    // ═══════════════════════════════════════════════════
    //  LOOKUP
    // ═══════════════════════════════════════════════════

    // Get RT by name (asserts if not found)
    VirtualResourceHandle GetRT(const char* name) const {
        shared_str key = name;
        auto it = m_registry.find(key);

        if (it == m_registry.end()) {
            R_ASSERT2(false, make_string("RT not found: %s", name));
        }

        return it->second;
    }

    // Try get RT (returns invalid handle if not found)
    VirtualResourceHandle TryGetRT(const char* name) const {
        shared_str key = name;
        auto it = m_registry.find(key);

        if (it != m_registry.end()) {
            return it->second;
        }

        return VirtualResourceHandle{};  // Invalid
    }

    // Check if RT exists
    bool HasRT(const char* name) const {
        shared_str key = name;
        return m_registry.find(key) != m_registry.end();
    }

    // ═══════════════════════════════════════════════════
    //  ENUMERATION
    // ═══════════════════════════════════════════════════

    // Get all registered names (for debugging)
    xr_vector<shared_str> GetAllNames() const {
        xr_vector<shared_str> names;
        names.reserve(m_registry.size());
        for (const auto& pair : m_registry) {
            names.push_back(pair.first);
        }
        return names;
    }

    // Print all registered RTs
    void PrintRegistry() const {
        Msg("! [RTRegistry] ═══ Registered render targets (%u total) ═══", m_registry.size());
        for (const auto& pair : m_registry) {
            Msg("!   '%s' -> Handle[%u]",
                pair.first.c_str(),
                pair.second.index);
        }
        Msg("! [RTRegistry] ═══════════════════════════════════════");
    }

    // ═══════════════════════════════════════════════════
    //  RESET
    // ═══════════════════════════════════════════════════

    void Clear() {
        m_registry.clear();
    }

private:
    // Use unordered_map for O(1) lookup instead of map's O(log n)
    std::unordered_map<shared_str, VirtualResourceHandle> m_registry;
};

} // namespace xray::render::framegraph

#pragma once

namespace xray::render::ng {

// ═══════════════════════════════════════════════════
//  OPAQUE RESOURCE HANDLES (Strongly Typed)
// ═══════════════════════════════════════════════════

constexpr u32 INVALID_INDEX = 0xFFFFFFFF;

// Base handle (not used directly)
struct ResourceHandle {
    u32 index = INVALID_INDEX;
    u32 generation = 0;  // For validation (detect stale handles)

    bool IsValid() const { return index != INVALID_INDEX; }

    bool operator==(const ResourceHandle& other) const {
        return index == other.index && generation == other.generation;
    }

    bool operator!=(const ResourceHandle& other) const {
        return !(*this == other);
    }

    bool operator<(const ResourceHandle& other) const {
        // For use in ordered maps
        if (index != other.index)
            return index < other.index;
        return generation < other.generation;
    }
};

// Strongly typed handles (prevent mixing)
struct BufferHandle : ResourceHandle {
    BufferHandle() = default;
    explicit BufferHandle(u32 idx, u32 gen = 0) {
        index = idx;
        generation = gen;
    }
};

struct TextureHandle : ResourceHandle {
    TextureHandle() = default;
    explicit TextureHandle(u32 idx, u32 gen = 0) {
        index = idx;
        generation = gen;
    }
};

struct SamplerHandle : ResourceHandle {
    SamplerHandle() = default;
    explicit SamplerHandle(u32 idx, u32 gen = 0) {
        index = idx;
        generation = gen;
    }
};

struct ShaderHandle : ResourceHandle {
    ShaderHandle() = default;
    explicit ShaderHandle(u32 idx, u32 gen = 0) {
        index = idx;
        generation = gen;
    }
};

struct PipelineStateHandle : ResourceHandle {
    PipelineStateHandle() = default;
    explicit PipelineStateHandle(u32 idx, u32 gen = 0) {
        index = idx;
        generation = gen;
    }
};

struct BindingLayoutHandle : ResourceHandle {
    BindingLayoutHandle() = default;
    explicit BindingLayoutHandle(u32 idx, u32 gen = 0) {
        index = idx;
        generation = gen;
    }
};

struct BindingSetHandle : ResourceHandle {
    BindingSetHandle() = default;
    explicit BindingSetHandle(u32 idx, u32 gen = 0) {
        index = idx;
        generation = gen;
    }
};

} // namespace xray::render::ng

// ═══════════════════════════════════════════════════
//  HASH SUPPORT (for xr_map/xr_unordered_map)
// ═══════════════════════════════════════════════════

namespace std {
    template<>
    struct hash<xray::render::ng::ResourceHandle> {
        size_t operator()(const xray::render::ng::ResourceHandle& h) const {
            // Combine index and generation into single hash
            return hash<u64>()((u64(h.generation) << 32) | u64(h.index));
        }
    };

    // Derived handles use same hash
    template<>
    struct hash<xray::render::ng::BufferHandle> {
        size_t operator()(const xray::render::ng::BufferHandle& h) const {
            return hash<xray::render::ng::ResourceHandle>()(h);
        }
    };

    template<>
    struct hash<xray::render::ng::TextureHandle> {
        size_t operator()(const xray::render::ng::TextureHandle& h) const {
            return hash<xray::render::ng::ResourceHandle>()(h);
        }
    };

    template<>
    struct hash<xray::render::ng::SamplerHandle> {
        size_t operator()(const xray::render::ng::SamplerHandle& h) const {
            return hash<xray::render::ng::ResourceHandle>()(h);
        }
    };

    template<>
    struct hash<xray::render::ng::ShaderHandle> {
        size_t operator()(const xray::render::ng::ShaderHandle& h) const {
            return hash<xray::render::ng::ResourceHandle>()(h);
        }
    };

    template<>
    struct hash<xray::render::ng::PipelineStateHandle> {
        size_t operator()(const xray::render::ng::PipelineStateHandle& h) const {
            return hash<xray::render::ng::ResourceHandle>()(h);
        }
    };
}

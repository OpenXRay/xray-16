#pragma once

#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include "xrCore/xrstring.h"

namespace xray::render::framegraph {

class VolatileConstantBufferPool {
public:
    VolatileConstantBufferPool();
    ~VolatileConstantBufferPool();

    struct CBLayout {
        shared_str name;
        u32 slot;
        u32 size;

        CBLayout() : slot(0), size(0) {}
        CBLayout(const char* n, u32 s, u32 sz) : name(n), slot(s), size(sz) {}

        bool operator<(const CBLayout& other) const {
            if (slot != other.slot) return slot < other.slot;
            if (size != other.size) return size < other.size;
            return xr_strcmp(name.c_str(), other.name.c_str()) < 0;
        }

        bool operator==(const CBLayout& other) const {
            return slot == other.slot &&
                   size == other.size &&
                   xr_strcmp(name.c_str(), other.name.c_str()) == 0;
        }
    };

    ng::BufferHandle GetOrCreateVCB(const CBLayout& layout);

    ng::BufferHandle GetVCB(const CBLayout& layout) const;

    bool HasLayout(const CBLayout& layout) const;

    struct Stats {
        u32 numLayouts = 0;
        u32 numVCBs = 0;
        u32 totalMemory = 0;
    };

    Stats GetStats() const;
    void LogStats() const;

private:
    xr_map<CBLayout, ng::BufferHandle> m_vcbs;

    mutable Stats m_stats;
};

}

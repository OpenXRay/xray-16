#include "stdafx.h"
#include "VolatileConstantBufferPool.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"

namespace xray::render::framegraph {

VolatileConstantBufferPool::VolatileConstantBufferPool()
{
    Msg("* [VCBPool] Created");
}

VolatileConstantBufferPool::~VolatileConstantBufferPool()
{
    Msg("* [VCBPool] Destroyed (%u unique layouts, %u VCBs, %u bytes total)",
        m_stats.numLayouts, m_stats.numVCBs, m_stats.totalMemory);
}

fg::BufferHandle VolatileConstantBufferPool::GetOrCreateVCB(const CBLayout& layout)
{
    auto it = m_vcbs.find(layout);
    if (it != m_vcbs.end())
    {
        return it->second;
    }

    Msg("* [VCBPool] Creating new VCB for '%s' (slot b%u, %u bytes)",
        layout.name.c_str(), layout.slot, layout.size);

    fg::RenderDevice::BufferDesc cbDesc;
    cbDesc.byteSize = layout.size;
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = 1024;

    string256 debugName;
    xr_sprintf(debugName, "VCB_%s_b%u_%ub", layout.name.c_str(), layout.slot, layout.size);
    cbDesc.debugName = debugName;

    fg::BufferHandle vcb = GEnv.Render->GetRenderDevice()->CreateBuffer(cbDesc);

    if (!vcb.IsValid())
    {
        Msg("! [VCBPool] Failed to create VCB for '%s'", layout.name.c_str());
        return fg::BufferHandle();
    }

    m_vcbs[layout] = vcb;

    m_stats.numLayouts++;
    m_stats.numVCBs++;
    m_stats.totalMemory += layout.size;

    Msg("  ✓ VCB created successfully (total: %u layouts, %u bytes)",
        m_stats.numLayouts, m_stats.totalMemory);

    return vcb;
}

fg::BufferHandle VolatileConstantBufferPool::GetVCB(const CBLayout& layout) const
{
    auto it = m_vcbs.find(layout);
    if (it != m_vcbs.end())
        return it->second;

    return fg::BufferHandle();
}

bool VolatileConstantBufferPool::HasLayout(const CBLayout& layout) const
{
    return m_vcbs.find(layout) != m_vcbs.end();
}

VolatileConstantBufferPool::Stats VolatileConstantBufferPool::GetStats() const
{
    return m_stats;
}

void VolatileConstantBufferPool::LogStats() const
{
    Msg("═══════════════════════════════════════════════════");
    Msg("  VCB POOL STATISTICS");
    Msg("═══════════════════════════════════════════════════");
    Msg("  Unique CB layouts: %u", m_stats.numLayouts);
    Msg("  VCB instances: %u", m_stats.numVCBs);
    Msg("  Total memory: %u bytes (%.2f KB)", m_stats.totalMemory, m_stats.totalMemory / 1024.0f);

    if (m_stats.numLayouts > 0)
    {
        Msg("  Average VCB size: %u bytes", m_stats.totalMemory / m_stats.numLayouts);
        Msg("");
        Msg("  Registered layouts:");

        for (const auto& pair : m_vcbs)
        {
            const CBLayout& layout = pair.first;
            Msg("    - '%s' (slot b%u, %u bytes)",
                layout.name.c_str(), layout.slot, layout.size);
        }
    }

    Msg("═══════════════════════════════════════════════════");
}

}

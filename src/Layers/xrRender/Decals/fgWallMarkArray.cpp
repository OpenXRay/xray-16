#include "stdafx.h"
#include "fgWallMarkArray.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"

namespace xray::render::fg {
    extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg::decals {

void fgWallMarkArray::Copy(IWallMarkArray& _in)
{
    auto& src = static_cast<fgWallMarkArray&>(_in);
    m_materialIDs = src.m_materialIDs;
    m_textureNames = src.m_textureNames;
    m_materialEpoch = src.m_materialEpoch;
}

u32 fgWallMarkArray::ResolveMaterial(u32 index)
{
    MaterialCache* materialCache = RImplementation.GetMaterialCache();
    if (index >= m_textureNames.size() || !materialCache)
        return UINT32_MAX;

    const u32 epoch = materialCache->GetVisualMaterialEpoch();
    if (m_materialEpoch != epoch)
    {
        std::fill(m_materialIDs.begin(), m_materialIDs.end(), UINT32_MAX);
        m_materialEpoch = epoch;
    }

    if (m_materialIDs[index] == UINT32_MAX)
        m_materialIDs[index] = materialCache->RegisterDecalMaterial(m_textureNames[index]);
    return m_materialIDs[index];
}

void fgWallMarkArray::AppendMark(LPCSTR s_textures)
{
    m_textureNames.emplace_back(s_textures);
    m_materialIDs.push_back(UINT32_MAX);
}

void fgWallMarkArray::clear()
{
    m_materialIDs.clear();
    m_textureNames.clear();
}

bool fgWallMarkArray::empty() { return m_textureNames.empty(); }

wm_shader fgWallMarkArray::GenerateWallmark()
{
    return {};
}

u32 fgWallMarkArray::GenerateBindlessMaterialID(shared_str* outTextureName)
{
    if (m_materialIDs.empty())
        return UINT32_MAX;
    u32 idx = ::Random.randI(0, m_materialIDs.size());
    if (outTextureName)
        *outTextureName = m_textureNames[idx];
    return ResolveMaterial(idx);
}

} // namespace xray::render::fg::decals

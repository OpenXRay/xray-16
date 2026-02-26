#include "stdafx.h"
#include "fgWallMarkArray.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/BindlessTypes.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::RENDER_NAMESPACE {
    class CRender;
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::decals {

void fgWallMarkArray::Copy(IWallMarkArray& _in)
{
    auto& src = static_cast<fgWallMarkArray&>(_in);
    m_materialIDs = src.m_materialIDs;
    m_textureNames = src.m_textureNames;
}

u32 fgWallMarkArray::TryRegisterMaterial(u32 index)
{
    if (index >= m_textureNames.size())
        return UINT32_MAX;

    auto* renderDevice = RImplementation.m_renderDevice;
    if (!renderDevice)
        return UINT32_MAX;
    auto* resMgr = renderDevice->GetFGResourceManager();
    if (!resMgr)
        return UINT32_MAX;
    auto* texMgr = resMgr->GetTextureManager();
    if (!texMgr)
        return UINT32_MAX;

    auto texHandle = texMgr->LoadTexture(m_textureNames[index].c_str());
    if (!texHandle.IsValid())
        return UINT32_MAX;

    nvrhi::ITexture* nvrhiTex = texMgr->GetNVRHITexture(texHandle);
    if (!nvrhiTex)
        return UINT32_MAX;

    u32 bindlessIdx = GEnv.Backend->RegisterBindlessTexture(nvrhiTex);
    if (bindlessIdx == UINT32_MAX)
        return UINT32_MAX;

    bindless::MaterialData mat = {};
    mat.diffuseIndex = bindlessIdx;
    mat.normalIndex = bindless::INVALID_TEXTURE_INDEX;
    mat.detailIndex = bindless::INVALID_TEXTURE_INDEX;
    mat.pbrIndex = bindless::INVALID_TEXTURE_INDEX;
    mat.detailScale = 1.0f;
    mat.alphaRef = 0.0f;
    mat.flags = 0;
    mat.shaderVariant = 0;
    u32 matID = bindless::MaterialBuffer::Instance().RegisterMaterial(mat);
    m_materialIDs[index] = matID;
    return matID;
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
    if (m_materialIDs[idx] == UINT32_MAX)
        TryRegisterMaterial(idx);
    return m_materialIDs[idx];
}

} // namespace xray::render::RENDER_NAMESPACE::decals

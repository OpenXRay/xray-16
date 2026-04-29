#include "stdafx.h"
#include "Layers/xrRender/Materials/ShaderInfo.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRenderDX11/Blender.h"
#include "Layers/xrRenderDX11/Blender_CLSID.h"
#include "Layers/xrRenderDX11/blenders/Blender_BmmD.h"
#include "Layers/xrRenderDX11/blenders/Blender_Particle.h"
#include "Layers/xrRenderDX11/ResourceManager.h"
#include "Layers/xrRender_R2/r2.h"

namespace xray::render::shader_info
{

static fg::CResourceManager* GetResources()
{
    return fg::RImplementation.Resources;
}

bool IsTerrainShader(const char* shaderName)
{
    if (!shaderName || !shaderName[0])
        return false;
    auto* res = GetResources();
    if (!res)
        return false;
    fg::IBlender* blender = res->_FindBlender(shaderName);
    if (!blender)
        return false;
    CLASS_ID cls = blender->getDescription().CLS;
    return (cls == fg::B_BmmD || cls == fg::B_LmBmmD);
}

bool GetTerrainDetailNames(const char* shaderName, TerrainDetailNames& out)
{
    if (!shaderName || !shaderName[0])
        return false;
    auto* res = GetResources();
    if (!res)
        return false;
    fg::IBlender* blender = res->_FindBlender(shaderName);
    if (!blender)
        return false;
    CLASS_ID cls = blender->getDescription().CLS;
    if (cls != fg::B_BmmD && cls != fg::B_LmBmmD)
        return false;
    auto* terrainBlender = static_cast<fg::CBlender_BmmD*>(blender);
    out.r = terrainBlender->GetDetailR();
    out.g = terrainBlender->GetDetailG();
    out.b = terrainBlender->GetDetailB();
    out.a = terrainBlender->GetDetailA();
    return true;
}

bool GetShaderBlendInfo(const char* shaderName, ShaderBlendInfo& out)
{
    if (!shaderName || !shaderName[0])
        return false;
    auto* res = GetResources();
    if (!res)
        return false;
    fg::CResourceManager::BlenderProperties props;
    if (!res->GetBlenderProperties(shaderName, props))
        return false;
    out.mode = static_cast<ShaderBlendMode>(props.blendMode);
    out.alphaRef = props.alphaRef;
    out.writesDepth = props.writesDepth;
    out.strictB2F = props.strictB2F;
    return true;
}

bool GetParticleBlendIndex(const char* shaderName, u32& outIndex)
{
    if (!shaderName || !shaderName[0])
        return false;
    auto* res = GetResources();
    if (!res)
        return false;
    fg::IBlender* B = res->_FindBlender(shaderName);
    if (!B)
        return false;
    if (B->getDescription().CLS != fg::B_PARTICLE)
        return false;
    auto* bp = static_cast<fg::CBlender_Particle*>(B);
    outIndex = bp->oBlend.IDselected;
    return true;
}

bool GetCompiledShaderNames(int shaderID, shared_str& outShaderName, shared_str& outTextureName)
{
    auto* compiled = fg::RImplementation.getCompiledShader(shaderID);
    if (!compiled)
        return false;
    outShaderName = compiled->shaderName;
    outTextureName = compiled->textureName;
    return true;
}

MaterialPSO* GetCompiledShaderPSO(int shaderID, u32 vertexFormatID, RenderPassType passType)
{
    auto* compiled = fg::RImplementation.getCompiledShader(shaderID);
    if (!compiled)
        return nullptr;
    u64 cacheKey = ((u64)vertexFormatID << 32) | (u64)passType;
    auto it = compiled->precompiledPSOs.psoCache.find(cacheKey);
    if (it != compiled->precompiledPSOs.psoCache.end())
        return it->second;
    return nullptr;
}

}

namespace xray::render::scene_info
{
const xr_vector<xray::render::fg::CSector*>& GetSceneSectors()
{
    return fg::RImplementation.get_imm_context().Sectors;
}
u32 GetPortalTraversalMarker()
{
    return fg::RImplementation.get_imm_context().PortalTraverser.i_marker;
}
}

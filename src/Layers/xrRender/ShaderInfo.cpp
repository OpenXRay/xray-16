#include "stdafx.h"
#include "Layers/xrRender/Materials/ShaderInfo.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/Blender.h"
#include "Layers/xrRender/Blender_CLSID.h"
#include "Layers/xrRender/blenders/Blender_BmmD.h"
#include "Layers/xrRender/blenders/Blender_Particle.h"
#include "Layers/xrRender/blenders/Blender_Screen_SET.h"
#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/r__scene.h"

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
    out.foliage = props.foliage;
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
    const CLASS_ID type = B->getDescription().CLS;
    if (type == fg::B_PARTICLE)
        outIndex = static_cast<fg::CBlender_Particle*>(B)->oBlend.IDselected;
    else if (type == fg::B_SCREEN_SET)
        outIndex = static_cast<fg::CBlender_Screen_SET*>(B)->oBlend.IDselected;
    else
        return false;
    return true;
}

bool GetCompiledShaderNames(int shaderID, shared_str& outShaderName, shared_str& outTextureName, shared_str& outLightmapName)
{
    auto* compiled = fg::RImplementation.getCompiledShader(shaderID);
    if (!compiled)
        return false;
    outShaderName = compiled->shaderName;
    outTextureName = compiled->textureName;
    outLightmapName = compiled->lightmapName;
    return true;
}

}

namespace xray::render::scene_info
{
const xr_vector<xray::render::fg::CSector*>& GetSceneSectors()
{
    return xray::render::fg::Scene.Sectors;
}
}

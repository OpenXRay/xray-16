#pragma once

namespace xray::render::shader_info
{

bool IsTerrainShader(const char* shaderName);

struct TerrainDetailNames
{
    const char* r = nullptr;
    const char* g = nullptr;
    const char* b = nullptr;
    const char* a = nullptr;
};
bool GetTerrainDetailNames(const char* shaderName, TerrainDetailNames& out);

enum class ShaderBlendMode : u32
{
    Opaque = 0,
    AlphaTest = 1,
    AlphaBlend = 2,
    Additive = 3,
    Multiply = 4,
    Multiply2X = 5,
};

struct ShaderBlendInfo
{
    ShaderBlendMode mode = ShaderBlendMode::Opaque;
    u32 alphaRef = 0;
    bool writesDepth = true;
    bool strictB2F = false;
};
bool GetShaderBlendInfo(const char* shaderName, ShaderBlendInfo& out);

bool GetParticleBlendIndex(const char* shaderName, u32& outIndex);

}

namespace xray::render
{
struct MaterialPSO;
enum class RenderPassType : u8;
}

namespace xray::render::shader_info
{
bool GetCompiledShaderNames(int shaderID, shared_str& outShaderName, shared_str& outTextureName);
xray::render::MaterialPSO* GetCompiledShaderPSO(int shaderID, u32 vertexFormatID, xray::render::RenderPassType passType);
}

namespace xray::render::fg
{
class CSector;
}

namespace xray::render::scene_info
{
const xr_vector<xray::render::fg::CSector*>& GetSceneSectors();
u32 GetPortalTraversalMarker();
}

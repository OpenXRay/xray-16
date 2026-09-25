#pragma once

#include "xrCore/PostProcess/PPInfo.hpp"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::resources
{
class TextureManager;
}

namespace xray::render::fg
{
class PostProcessFrame
{
public:
    bool active = false;
    float blur = 0.0f;
    Fvector2 dualityShift = {};
    float gray = 0.0f;
    Fvector colorGray = {};
    float noiseIntensity = 0.0f;
    Fvector2 noiseScale = {};
    Fvector2 noiseOffset = {};
    Fvector colorBase = {};
    Fvector colorAdd = {};
    float colorMapInfluence = 0.0f;
    float colorMapInterpolate = 0.0f;
    nvrhi::TextureHandle noise;
    nvrhi::TextureHandle colorMap0;
    nvrhi::TextureHandle colorMap1;
};

class PostProcessEffects
{
public:
    void SetParams(const SPPInfo& params);
    PostProcessFrame Prepare(resources::TextureManager* textures, float deltaTime);
    void ReleaseTextures(resources::TextureManager* textures);

private:
    nvrhi::ITexture* AcquireTexture(resources::TextureManager& textures, const shared_str& name);
    void AdvanceNoise(float deltaTime, float fps);
    void ResolveNoise(resources::TextureManager& textures, float deltaTime, PostProcessFrame& frame);
    void ResolveColorMaps(resources::TextureManager& textures, PostProcessFrame& frame);

    SPPInfo m_params;
    xr_map<shared_str, TextureHandle> m_textures;
    float m_noiseTime = 0.0f;
    Fvector2 m_noiseOffset = {};
};
}

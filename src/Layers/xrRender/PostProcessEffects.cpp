#include "stdafx.h"
#include "PostProcessEffects.h"
#include "Layers/xrRender/ColorSpace.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"

namespace xray::render::fg
{
namespace
{
constexpr pcstr kNoiseTexture = "fx" DELIMITER "fx_noise2";
constexpr float kEffectThreshold = 0.001f;
constexpr float kColorThreshold = 0.5f / 255.0f;

float Saturate(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

Fvector Saturate(const SPPInfo::SColor& color)
{
    return Fvector().set(Saturate(color.r), Saturate(color.g), Saturate(color.b));
}

bool Differs(const Fvector& color, float reference)
{
    return _abs(color.x - reference) > kColorThreshold
        || _abs(color.y - reference) > kColorThreshold
        || _abs(color.z - reference) > kColorThreshold;
}
}

void PostProcessEffects::SetParams(const SPPInfo& params)
{
    m_params = params;
}

PostProcessFrame PostProcessEffects::Prepare(resources::TextureManager* textures, float deltaTime)
{
    PostProcessFrame frame;
    frame.blur = m_params.blur;
    frame.dualityShift.set(_abs(m_params.duality.h) * 0.5f, _abs(m_params.duality.v) * 0.5f);
    frame.gray = Saturate(m_params.gray);
    frame.colorGray = Saturate(m_params.color_gray);
    frame.noiseIntensity = Saturate(m_params.noise.intensity);
    frame.colorBase = Saturate(m_params.color_base);
    frame.colorAdd.set(m_params.color_add.r, m_params.color_add.g, m_params.color_add.b);
    frame.colorMapInfluence = Saturate(m_params.cm_influence);
    frame.colorMapInterpolate = Saturate(m_params.cm_interpolate);

    if (textures)
    {
        ResolveNoise(*textures, deltaTime, frame);
        ResolveColorMaps(*textures, frame);
    }
    else
    {
        frame.noiseIntensity = 0.0f;
        frame.colorMapInfluence = 0.0f;
    }

    frame.active = _abs(frame.blur) > kEffectThreshold
        || _abs(m_params.duality.h) > kEffectThreshold
        || _abs(m_params.duality.v) > kEffectThreshold
        || frame.gray > kEffectThreshold
        || frame.noiseIntensity > 0.0f
        || frame.colorMapInfluence > 0.0f
        || Differs(frame.colorBase, 0.5f)
        || Differs(frame.colorAdd, 0.0f);
    return frame;
}

void PostProcessEffects::ReleaseTextures(resources::TextureManager* textures)
{
    if (textures)
    {
        for (const auto& entry : m_textures)
            textures->Release(entry.second);
    }
    m_textures.clear();
}

nvrhi::ITexture* PostProcessEffects::AcquireTexture(resources::TextureManager& textures, const shared_str& name)
{
    auto found = m_textures.find(name);
    if (found == m_textures.end())
        found = m_textures.emplace(name, textures.LoadTexture(name.c_str(), TextureColorSpace::Linear)).first;
    return textures.GetNVRHITexture(found->second);
}

void PostProcessEffects::AdvanceNoise(float deltaTime, float fps)
{
    m_noiseTime -= deltaTime;
    if (m_noiseTime >= 0.0f)
        return;

    m_noiseOffset.set(::Random.randF(), ::Random.randF());
    const float period = 1.0f / (_abs(fps) + EPS_S);
    m_noiseTime = period - std::fmod(-m_noiseTime, period);
}

void PostProcessEffects::ResolveNoise(resources::TextureManager& textures, float deltaTime, PostProcessFrame& frame)
{
    if (frame.noiseIntensity <= kEffectThreshold)
    {
        frame.noiseIntensity = 0.0f;
        return;
    }

    nvrhi::ITexture* noise = AcquireTexture(textures, shared_str(kNoiseTexture));
    if (!noise)
    {
        frame.noiseIntensity = 0.0f;
        return;
    }

    AdvanceNoise(deltaTime, m_params.noise.fps);
    const nvrhi::TextureDesc& desc = noise->getDesc();
    const float grain = std::max(m_params.noise.grain, EPS_L);
    frame.noise = noise;
    frame.noiseScale.set(1.0f / (float(desc.width) * grain), 1.0f / (float(desc.height) * grain));
    frame.noiseOffset = m_noiseOffset;
}

void PostProcessEffects::ResolveColorMaps(resources::TextureManager& textures, PostProcessFrame& frame)
{
    if (frame.colorMapInfluence <= kEffectThreshold || !m_params.cm_tex1.size())
    {
        frame.colorMapInfluence = 0.0f;
        return;
    }

    nvrhi::ITexture* first = AcquireTexture(textures, m_params.cm_tex1);
    if (!first)
    {
        frame.colorMapInfluence = 0.0f;
        return;
    }

    nvrhi::ITexture* second = m_params.cm_tex2.size() ? AcquireTexture(textures, m_params.cm_tex2) : nullptr;
    frame.colorMap0 = first;
    frame.colorMap1 = second ? second : first;
}
}

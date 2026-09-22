#pragma once

#include "xrEngine/DeveloperScene.h"

class CDeveloperRoom
{
public:
    static void Build(xray::render::DeveloperScene& scene);
    static Fvector ActorPosition();

private:
    static u32 AddMaterial(xray::render::DeveloperScene& scene, pcstr name,
        float red, float green, float blue, float metallic, float roughness,
        pcstr shaderName = nullptr, float opacity = 1.0f);
    static void AddBox(xray::render::DeveloperScene& scene, pcstr name,
        const Fvector& center, const Fvector& halfSize, u32 material, bool collision = true);
    static void AddSphere(xray::render::DeveloperScene& scene, pcstr name,
        const Fvector& center, float radius, u32 material);
    static void AddFace(xray::render::DeveloperSceneMesh& mesh, const Fvector& center,
        const Fvector& normal, const Fvector& tangent, const Fvector& binormal,
        float width, float height);
    static void AddGlassDisplay(xray::render::DeveloperScene& scene, pcstr name,
        const Fvector& center, const Fvector& halfSize, u32 glass, u32 frame);
    static void AddPanelDisplay(xray::render::DeveloperScene& scene, pcstr name,
        const Fvector& center, const Fvector& halfSize, u32 panel, u32 support);
    static void AddPlinthSample(xray::render::DeveloperScene& scene, pcstr name,
        const Fvector& center, float radius, float height, u32 material, u32 plinth);
    static void AddSampleRecord(pcstr group, const xray::render::DeveloperSceneMaterial& material,
        const Fvector& position);
};

#pragma once

#include "xrEngine/DeveloperScene.h"

class CDeveloperRoom
{
public:
    static void Build(xray::render::DeveloperScene& scene);
    static Fvector ActorPosition();

private:
    static u32 AddMaterial(xray::render::DeveloperScene& scene, pcstr name,
        float red, float green, float blue, float metallic, float roughness);
    static void AddBox(xray::render::DeveloperScene& scene, pcstr name,
        const Fvector& center, const Fvector& halfSize, u32 material, bool collision = true);
    static void AddSphere(xray::render::DeveloperScene& scene, pcstr name,
        const Fvector& center, float radius, u32 material);
    static void AddFace(xray::render::DeveloperSceneMesh& mesh, const Fvector& center,
        const Fvector& normal, const Fvector& tangent, const Fvector& binormal,
        float width, float height);
};

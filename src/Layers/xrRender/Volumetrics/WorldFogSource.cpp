#include "stdafx.h"
#include "WorldFogSource.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"

namespace xray::render::fg
{

void WorldFogSource::PrepareGPUData()
{
    if (!g_pGamePersistent)
        return;

    const auto& env = g_pGamePersistent->Environment().CurrentEnv;
    // env.fog_density is a unitless 0..1 blend factor (drives fog_near), NOT an
    // extinction coefficient. Map it to a soft physical density so a full-length
    // sky ray does not crush transmittance to ~0 (black sky).
    const float fogFar = std::max(env.fog_distance, 50.f);
    const float density01 = std::min(std::max(env.fog_density, 0.f), 1.f);
    // Softer than MVP: optical depth ~0.2 at fogFar (shafts carry volumetric look)
    const float targetOD = 0.2f * density01;
    m_params.density = targetOD / fogFar;
    m_params.heightFalloff = 0.01f + density01 * 0.015f;
    m_params.baseHeight = 0.0f;
    m_params.albedo.set(env.fog_color.x, env.fog_color.y, env.fog_color.z);
}

void WorldFogSource::GetWorldBounds(Fvector& outMin, Fvector& outMax) const
{
    outMin.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    outMax.set(FLT_MAX, FLT_MAX, FLT_MAX);
}

} // namespace xray::render::fg

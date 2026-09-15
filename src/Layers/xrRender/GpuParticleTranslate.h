#pragma once

#include "xrCore/xrCore.h"
#include "../../../res/gamedata/shaders/r5/gpu_particle_types.h"

namespace xray::render::fg
{
namespace PS
{
class CPEDef;
}

bool TranslateGpuParticleDefinition(const PS::CPEDef& definition, GpuPapiProgram& program,
    xr_vector<GpuPapiAction>& actions, xr_string& error);
}

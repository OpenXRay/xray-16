// xrRender/ShaderKey.cpp
#include "stdafx.h"
#include "ShaderKey.h"
#include "Shader.h"
#include "FBasicVisual.h"
#include "SH_Atomic.h"

namespace xray::render::RENDER_NAMESPACE {

bool ExtractShaderKey(dxRender_Visual* visual, ShaderKey& outKey) {
    if (!visual) {
        return false;
    }

    // Get shader from visual
    Shader* shader = visual->shader._get();
    if (!shader) {
        return false;
    }

    // Get first shader element
    ShaderElement* elem = shader->E[0]._get();
    if (!elem) {
        return false;
    }

    // Get first pass
    if (elem->passes.empty()) {
        return false;
    }

    SPass* pass = elem->passes[0]._get();
    if (!pass) {
        return false;
    }

    return ExtractShaderKeyFromPass(pass, outKey);
}

bool ExtractShaderKeyFromPass(SPass* pass, ShaderKey& outKey) {
    if (!pass) {
        return false;
    }

    // Clear the output key
    outKey = ShaderKey{};

    // Extract vertex shader name
    if (pass->vs._get()) {
        outKey.vsName = pass->vs->cName;
    }

    // Extract pixel shader name
    if (pass->ps._get()) {
        outKey.psName = pass->ps->cName;
    }

    // Extract geometry shader name
    if (pass->gs._get()) {
        outKey.gsName = pass->gs->cName;
    }

#ifdef USE_DX11
    // Extract hull shader name
    if (pass->hs._get()) {
        outKey.hsName = pass->hs->cName;
    }

    // Extract domain shader name
    if (pass->ds._get()) {
        outKey.dsName = pass->ds->cName;
    }

    // Extract compute shader name
    if (pass->cs._get()) {
        outKey.csName = pass->cs->cName;
    }
#elif defined(USE_OGL)
    // Extract program pipeline name
    if (pass->pp._get()) {
        outKey.ppName = pass->pp->cName;
    }
#endif

    // Verify we got at least one shader
    return outKey.IsValid();
}

} // namespace xray::render::RENDER_NAMESPACE

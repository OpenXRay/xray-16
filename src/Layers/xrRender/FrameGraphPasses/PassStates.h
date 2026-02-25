#pragma once

#include "TonemapPassSetup.h"
#include "DetailPassSetup.h"
#include "SunPassSetup.h"
#include "SkyPassSetup.h"
#include "TransparentPassSetup.h"
#include "HiZBuildPassSetup.h"
#include "ExposurePassSetup.h"
#include "UIPassSetup.h"
#include "ParticlePassSetup.h"
#include "ForwardColorPassSetup.h"
#include "SkinningPassSetup.h"
#include "MotionVectorPassSetup.h"
#include "ReSTIRGIPassSetup.h"
#include "DecalPassSetup.h"
#include "OverlayPaintPassSetup.h"

namespace xray::render::RENDER_NAMESPACE::passes {

struct PassStates {
    TonemapPassState tonemap;
    DetailPassState detail;
    SunPassState sun;
    SkyPassState sky;
    TransparentPassState transparent;
    HiZBuildPassState hiZBuild;
    ExposurePassState exposure;
    UITextPassState uiText;
    ParticlePassState particle;
    ForwardColorPassState forwardColor;
    SkinningPassState skinning;
    MotionVectorPassState motionVector;
    ReSTIRGIPassState restirGI;
    DecalPassState decal;
    OverlayPaintPassState overlayPaint;
};

} // namespace xray::render::RENDER_NAMESPACE::passes

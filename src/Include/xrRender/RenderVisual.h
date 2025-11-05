#ifndef RenderVisual_included
#define RenderVisual_included
#pragma once

#include "xrCommon/xr_vector.h"
#include "xrCore/_matrix.h"

class IKinematics;
class IKinematicsAnimated;
class IParticleCustom;
struct vis_data;

namespace xray::render::RENDER_NAMESPACE
{
class COzzKinematicsVisual;
}

class XR_NOVTABLE IRenderVisual
{
public:
    virtual ~IRenderVisual() = 0;
    virtual vis_data& getVisData() = 0;
    virtual u32 getType() const = 0;

    virtual void DebugDumpPalette(const xr_vector<Fmatrix>& palette) const {}

#ifdef DEBUG
    virtual shared_str getDebugName() = 0;
#endif

    virtual IRenderVisual* getSubModel(u8 idx) { return nullptr; } //--#SM+#--
    virtual IKinematics* dcast_PKinematics() { return nullptr; }
    virtual IKinematicsAnimated* dcast_PKinematicsAnimated() { return nullptr; }
    virtual IParticleCustom* dcast_ParticleCustom() { return nullptr; }
    virtual xray::render::RENDER_NAMESPACE::COzzKinematicsVisual* dcast_OzzKinematics() { return nullptr; }
};

inline IRenderVisual::~IRenderVisual() = default;

#endif //	RenderVisual_included

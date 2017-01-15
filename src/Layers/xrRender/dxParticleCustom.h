//---------------------------------------------------------------------------
#ifndef ParticleCustomH
#define ParticleCustomH

#include "FBasicVisual.h"
#include "Include/xrRender/ParticleCustom.h"
//---------------------------------------------------------------------------
class dxParticleCustom : public dxRender_Visual, public IParticleCustom
{
public:
    // geometry-format
    ref_geom geom;

public:
    virtual ~dxParticleCustom() { ; }
    virtual IParticleCustom* dcast_ParticleCustom() { return this; }
};

//---------------------------------------------------------------------------
#endif  // ParticleCustomH

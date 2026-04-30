#include "stdafx.h"
#include "dxRenderFactory.h"

#include "dxStatGraphRender.h"
#ifndef _EDITOR
#include "dxLensFlareRender.h"
#include "Layers/xrRenderDX11/dxImGuiRender.h"
#endif
#ifndef _EDITOR
#include "dxThunderboltRender.h"
#include "dxThunderboltDescRender.h"
#include "dxRainRender.h"
#include "dxLensFlareRender.h"
#include "dxEnvironmentRender.h"
#include "dxObjectSpaceRender.h"
#endif // _EDITOR

#include "dxFontRender.h"
#include "dxWallMarkArray.h"
#include "Decals/fgWallMarkArray.h"
#include "dxUISequenceVideoItem.h"
#include "dxUIShader.h"

namespace xray::render::fg
{
dxRenderFactory RenderFactoryImpl;

#define RENDER_FACTORY_IMPLEMENT(Class)\
    I##Class* dxRenderFactory::Create##Class()\
    { return xr_new<dx##Class>(); }\
    void dxRenderFactory::Destroy##Class(I##Class* pObject)\
    { xr_delete((dx##Class*&)pObject); }

#ifndef _EDITOR
RENDER_FACTORY_IMPLEMENT(UISequenceVideoItem)
RENDER_FACTORY_IMPLEMENT(UIShader)
RENDER_FACTORY_IMPLEMENT(StatGraphRender)
#ifdef DEBUG
RENDER_FACTORY_IMPLEMENT(ObjectSpaceRender)
#endif // DEBUG
IWallMarkArray* dxRenderFactory::CreateWallMarkArray()
{
    return xr_new<decals::fgWallMarkArray>();
}
void dxRenderFactory::DestroyWallMarkArray(IWallMarkArray* pObject)
{
    auto* p = static_cast<decals::fgWallMarkArray*>(pObject);
    xr_delete(p);
}
#endif // _EDITOR

#ifndef _EDITOR
RENDER_FACTORY_IMPLEMENT(ThunderboltRender)
RENDER_FACTORY_IMPLEMENT(ThunderboltDescRender)
RENDER_FACTORY_IMPLEMENT(RainRender)
RENDER_FACTORY_IMPLEMENT(LensFlareRender)
RENDER_FACTORY_IMPLEMENT(EnvironmentRender)
RENDER_FACTORY_IMPLEMENT(EnvDescriptorRender)
RENDER_FACTORY_IMPLEMENT(FlareRender)
#endif
RENDER_FACTORY_IMPLEMENT(FontRender)
} // namespace xray::render::fg

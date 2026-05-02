#include "stdafx.h"
#include "dxRenderFactory.h"

#include "fgStatGraphRender.h"
#ifndef _EDITOR
#include "Layers/xrRenderDX11/dxImGuiRender.h"
#endif
#ifndef _EDITOR
#include "dxThunderboltDescRender.h"
#include "fgThunderboltRender.h"
#include "fgRainRender.h"
#include "fgLensFlareRender.h"
#include "fgEnvironmentRender.h"
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
IStatGraphRender* dxRenderFactory::CreateStatGraphRender()
{
    return xr_new<FGStatGraphRender>();
}
void dxRenderFactory::DestroyStatGraphRender(IStatGraphRender* pObject)
{
    auto* p = static_cast<FGStatGraphRender*>(pObject);
    xr_delete(p);
}
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
IThunderboltRender* dxRenderFactory::CreateThunderboltRender()
{
    return xr_new<FGThunderboltRender>();
}
void dxRenderFactory::DestroyThunderboltRender(IThunderboltRender* pObject)
{
    auto* p = static_cast<FGThunderboltRender*>(pObject);
    xr_delete(p);
}
RENDER_FACTORY_IMPLEMENT(ThunderboltDescRender)
IRainRender* dxRenderFactory::CreateRainRender()
{
    return xr_new<FGRainRender>();
}
void dxRenderFactory::DestroyRainRender(IRainRender* pObject)
{
    auto* p = static_cast<FGRainRender*>(pObject);
    xr_delete(p);
}
ILensFlareRender* dxRenderFactory::CreateLensFlareRender()
{
    return xr_new<FGLensFlareRender>();
}
void dxRenderFactory::DestroyLensFlareRender(ILensFlareRender* pObject)
{
    auto* p = static_cast<FGLensFlareRender*>(pObject);
    xr_delete(p);
}
IEnvironmentRender* dxRenderFactory::CreateEnvironmentRender()
{
    return xr_new<FGEnvironmentRender>();
}
void dxRenderFactory::DestroyEnvironmentRender(IEnvironmentRender* pObject)
{
    auto* p = static_cast<FGEnvironmentRender*>(pObject);
    xr_delete(p);
}
IEnvDescriptorRender* dxRenderFactory::CreateEnvDescriptorRender()
{
    return xr_new<FGEnvDescriptorRender>();
}
void dxRenderFactory::DestroyEnvDescriptorRender(IEnvDescriptorRender* pObject)
{
    auto* p = static_cast<FGEnvDescriptorRender*>(pObject);
    xr_delete(p);
}
RENDER_FACTORY_IMPLEMENT(FlareRender)
#endif
RENDER_FACTORY_IMPLEMENT(FontRender)
} // namespace xray::render::fg

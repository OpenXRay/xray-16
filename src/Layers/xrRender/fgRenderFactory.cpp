#include "stdafx.h"
#include "fgRenderFactory.h"

#include "fgStatGraphRender.h"
#ifndef _EDITOR
#endif
#ifndef _EDITOR
#include "fgThunderboltDescRender.h"
#include "fgThunderboltRender.h"
#include "fgRainRender.h"
#include "fgLensFlareRender.h"
#include "fgFlareRender.h"
#include "fgEnvironmentRender.h"
#include "fgObjectSpaceRender.h"
#endif // _EDITOR

#include "fgFontRender.h"
#include "Decals/fgWallMarkArray.h"
#include "fgUISequenceVideoItem.h"
#include "fgUIShader.h"

namespace xray::render::fg
{
fgRenderFactory RenderFactoryImpl;

#ifndef _EDITOR
IUISequenceVideoItem* fgRenderFactory::CreateUISequenceVideoItem()
{
    return xr_new<fgUISequenceVideoItem>();
}
void fgRenderFactory::DestroyUISequenceVideoItem(IUISequenceVideoItem* pObject)
{
    auto* p = static_cast<fgUISequenceVideoItem*>(pObject);
    xr_delete(p);
}

IUIShader* fgRenderFactory::CreateUIShader()
{
    return xr_new<fgUIShader>();
}
void fgRenderFactory::DestroyUIShader(IUIShader* pObject)
{
    auto* p = static_cast<fgUIShader*>(pObject);
    xr_delete(p);
}

IStatGraphRender* fgRenderFactory::CreateStatGraphRender()
{
    return xr_new<FGStatGraphRender>();
}
void fgRenderFactory::DestroyStatGraphRender(IStatGraphRender* pObject)
{
    auto* p = static_cast<FGStatGraphRender*>(pObject);
    xr_delete(p);
}

#ifdef DEBUG
IObjectSpaceRender* fgRenderFactory::CreateObjectSpaceRender()
{
    return xr_new<fgObjectSpaceRender>();
}
void fgRenderFactory::DestroyObjectSpaceRender(IObjectSpaceRender* pObject)
{
    auto* p = static_cast<fgObjectSpaceRender*>(pObject);
    xr_delete(p);
}
#endif
IWallMarkArray* fgRenderFactory::CreateWallMarkArray()
{
    return xr_new<decals::fgWallMarkArray>();
}
void fgRenderFactory::DestroyWallMarkArray(IWallMarkArray* pObject)
{
    auto* p = static_cast<decals::fgWallMarkArray*>(pObject);
    xr_delete(p);
}
#endif // _EDITOR

#ifndef _EDITOR
IThunderboltRender* fgRenderFactory::CreateThunderboltRender()
{
    return xr_new<FGThunderboltRender>();
}
void fgRenderFactory::DestroyThunderboltRender(IThunderboltRender* pObject)
{
    auto* p = static_cast<FGThunderboltRender*>(pObject);
    xr_delete(p);
}
IThunderboltDescRender* fgRenderFactory::CreateThunderboltDescRender()
{
    return xr_new<fgThunderboltDescRender>();
}
void fgRenderFactory::DestroyThunderboltDescRender(IThunderboltDescRender* pObject)
{
    auto* p = static_cast<fgThunderboltDescRender*>(pObject);
    xr_delete(p);
}
IRainRender* fgRenderFactory::CreateRainRender()
{
    return xr_new<FGRainRender>();
}
void fgRenderFactory::DestroyRainRender(IRainRender* pObject)
{
    auto* p = static_cast<FGRainRender*>(pObject);
    xr_delete(p);
}
ILensFlareRender* fgRenderFactory::CreateLensFlareRender()
{
    return xr_new<FGLensFlareRender>();
}
void fgRenderFactory::DestroyLensFlareRender(ILensFlareRender* pObject)
{
    auto* p = static_cast<FGLensFlareRender*>(pObject);
    xr_delete(p);
}
IEnvironmentRender* fgRenderFactory::CreateEnvironmentRender()
{
    return xr_new<FGEnvironmentRender>();
}
void fgRenderFactory::DestroyEnvironmentRender(IEnvironmentRender* pObject)
{
    auto* p = static_cast<FGEnvironmentRender*>(pObject);
    xr_delete(p);
}
IEnvDescriptorRender* fgRenderFactory::CreateEnvDescriptorRender()
{
    return xr_new<FGEnvDescriptorRender>();
}
void fgRenderFactory::DestroyEnvDescriptorRender(IEnvDescriptorRender* pObject)
{
    auto* p = static_cast<FGEnvDescriptorRender*>(pObject);
    xr_delete(p);
}
IFlareRender* fgRenderFactory::CreateFlareRender()
{
    return xr_new<FGFlareRender>();
}
void fgRenderFactory::DestroyFlareRender(IFlareRender* pObject)
{
    auto* p = static_cast<FGFlareRender*>(pObject);
    xr_delete(p);
}
#endif
IFontRender* fgRenderFactory::CreateFontRender()
{
    return xr_new<FGFontRender>();
}
void fgRenderFactory::DestroyFontRender(IFontRender* pObject)
{
    auto* p = static_cast<FGFontRender*>(pObject);
    xr_delete(p);
}
} // namespace xray::render::fg

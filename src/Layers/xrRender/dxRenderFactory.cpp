#include "stdafx.h"
#include "dxRenderFactory.h"
#include "dxStatGraphRender.h"

#ifndef _EDITOR
#include "dxLensFlareRender.h"
#include "dxImGuiRender.h"
#include "dxEnvironmentRender.h"
#include "dxThunderboltRender.h"
#include "dxThunderboltDescRender.h"
#include "dxRainRender.h"
#include "dxLensFlareRender.h"
#include "dxObjectSpaceRender.h"
#endif // _EDITOR

#include "dxFontRender.h"
#include "dxWallMarkArray.h"
#include "dxUISequenceVideoItem.h"
#include "dxUIShader.h"

dxRenderFactory RenderFactoryImpl;

#define RENDER_FACTORY_IMPLEMENT(Class)\
    I##Class* dxRenderFactory::Create##Class()\
    { return xr_new<dx##Class>(); }\
    void dxRenderFactory::Destroy##Class(I##Class* pObject)\
    { xr_delete((dx##Class*&)pObject); }

#ifndef _EDITOR
RENDER_FACTORY_IMPLEMENT(UISequenceVideoItem)
RENDER_FACTORY_IMPLEMENT(UIShader)
#ifdef DEBUG
RENDER_FACTORY_IMPLEMENT(ObjectSpaceRender)
#endif // DEBUG
RENDER_FACTORY_IMPLEMENT(WallMarkArray)
#endif // _EDITOR

#ifndef _EDITOR
RENDER_FACTORY_IMPLEMENT(ThunderboltRender)
RENDER_FACTORY_IMPLEMENT(ThunderboltDescRender)
RENDER_FACTORY_IMPLEMENT(RainRender)
RENDER_FACTORY_IMPLEMENT(LensFlareRender)
RENDER_FACTORY_IMPLEMENT(ImGuiRender)
RENDER_FACTORY_IMPLEMENT(FlareRender)
RENDER_FACTORY_IMPLEMENT(EnvironmentRender)
RENDER_FACTORY_IMPLEMENT(EnvDescriptorRender)
#endif
RENDER_FACTORY_IMPLEMENT(FontRender)
RENDER_FACTORY_IMPLEMENT(StatGraphRender)

#include "stdafx.h"
#include "dxRenderFactory.h"

#include "dxStatGraphRender.h"
#ifndef _EDITOR
#include "dxLensFlareRender.h"
#endif
#include "dxConsoleRender.h"
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
#include "dxUISequenceVideoItem.h"
#include "dxUIShader.h"

dxRenderFactory RenderFactoryImpl;

#define RENDER_FACTORY_IMPLEMENT(Class)\
    I##Class* dxRenderFactory::Create##Class()\
    { return new dx##Class(); }\
    void dxRenderFactory::Destroy##Class(I##Class* pObject)\
    { xr_delete((dx##Class*&)pObject); }

#ifndef _EDITOR
RENDER_FACTORY_IMPLEMENT(UISequenceVideoItem)
RENDER_FACTORY_IMPLEMENT(UIShader)
RENDER_FACTORY_IMPLEMENT(StatGraphRender)
RENDER_FACTORY_IMPLEMENT(ConsoleRender)
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
RENDER_FACTORY_IMPLEMENT(EnvironmentRender)
RENDER_FACTORY_IMPLEMENT(EnvDescriptorMixerRender)
RENDER_FACTORY_IMPLEMENT(EnvDescriptorRender)
RENDER_FACTORY_IMPLEMENT(FlareRender)
#endif
RENDER_FACTORY_IMPLEMENT(FontRender)

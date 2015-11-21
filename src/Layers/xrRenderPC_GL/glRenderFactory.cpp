#include "stdafx.h"
#include "glRenderFactory.h"

//#include "glStatGraphRender.h"
#ifndef _EDITOR
	#include "glLensFlareRender.h"
#endif
//#include "glConsoleRender.h"
#ifndef _EDITOR
	#include "glThunderboltRender.h"
	#include "glThunderboltDescRender.h"
	#include "glRainRender.h"
	#include "glLensFlareRender.h"
	#include "glEnvironmentRender.h"

	#include "glRenderDeviceRender.h"
	#include "glObjectSpaceRender.h"
#endif // _EDITOR

#include "glFontRender.h"
#include "glApplicationRender.h"
#include "glWallMarkArray.h"
#include "glStatsRender.h"
#include "glUISequenceVideoItem.h"
#include "glUIShader.h"

glRenderFactory RenderFactoryImpl;

//# include "gl##Class.h" \

#define RENDER_FACTORY_IMPLEMENT(Class) \
	I##Class* glRenderFactory::Create##Class() \
{ \
	return new gl##Class(); \
} \
	void glRenderFactory::Destroy##Class(I##Class *pObject)\
{ \
	xr_delete((gl##Class*&)pObject); \
} \

#define RENDER_FACTORY_UNIMPLEMENT(Class) \
	I##Class* glRenderFactory::Create##Class() \
{ \
	VERIFY(!#Class" not implemented."); return nullptr; \
} \
	void glRenderFactory::Destroy##Class(I##Class *pObject)\
{ \
} \

#ifndef _EDITOR
	RENDER_FACTORY_IMPLEMENT(UISequenceVideoItem)
	RENDER_FACTORY_IMPLEMENT(UIShader)
	RENDER_FACTORY_UNIMPLEMENT(StatGraphRender)
	RENDER_FACTORY_UNIMPLEMENT(ConsoleRender)
	RENDER_FACTORY_IMPLEMENT(RenderDeviceRender)
#	ifdef DEBUG
		RENDER_FACTORY_IMPLEMENT(ObjectSpaceRender)
#	endif // DEBUG
	RENDER_FACTORY_IMPLEMENT(ApplicationRender)
	RENDER_FACTORY_IMPLEMENT(WallMarkArray)
	RENDER_FACTORY_IMPLEMENT(StatsRender)
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

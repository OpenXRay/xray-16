#include "pch.h"
#undef RENDER_FACTORY_INTERFACE
#define RENDER_FACTORY_INTERFACE(Class)\
I ## Class* XRayRenderFactory::Create ## Class()\
{\
	return static_cast<I ## Class*>(new XRay ## Class);\
}\
void XRayRenderFactory::Destroy##Class(I ## Class *pObject)\
{\
	delete  static_cast< XRay ## Class*>(pObject);\
}
#ifndef _EDITOR
RENDER_FACTORY_INTERFACE(UISequenceVideoItem)
RENDER_FACTORY_INTERFACE(UIShader)
RENDER_FACTORY_INTERFACE(StatGraphRender)
RENDER_FACTORY_INTERFACE(ConsoleRender)
#	ifdef DEBUG
RENDER_FACTORY_INTERFACE(ObjectSpaceRender)
#	endif // DEBUG
RENDER_FACTORY_INTERFACE(WallMarkArray)
#endif // _EDITOR

RENDER_FACTORY_INTERFACE(RainRender)
RENDER_FACTORY_INTERFACE(EnvDescriptorMixerRender)
#ifndef _EDITOR
RENDER_FACTORY_INTERFACE(EnvironmentRender)
RENDER_FACTORY_INTERFACE(EnvDescriptorRender)
RENDER_FACTORY_INTERFACE(ThunderboltRender)
RENDER_FACTORY_INTERFACE(ThunderboltDescRender)
RENDER_FACTORY_INTERFACE(FlareRender)
#endif // _EDITOR
RENDER_FACTORY_INTERFACE(FontRender)
RENDER_FACTORY_INTERFACE(LensFlareRender)

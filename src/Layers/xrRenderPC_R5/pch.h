#include "../../xrEngine/stdafx.h"
#include "../../xrEngine/Render.h"

#include "../../Include/xrRender/RenderFactory.h"
#include "../../Include/xrRender/UISequenceVideoItem.h"
#include "../../Include/xrRender/ConsoleRender.h"
#include "../../Include/xrRender/StatGraphRender.h"
#include "../../Include/xrRender/EnvironmentRender.h"
#include "../../Include/xrRender/LensFlareRender.h"
#include "../../Include/xrRender/RainRender.h"
#include "../../Include/xrRender/ThunderboltDescRender.h"
#include "../../Include/xrRender/ThunderboltRender.h"
#include "../../Include/xrRender/WallMarkArray.h"
#include "../../Include/xrRender/ObjectSpaceRender.h"
#include "../../Include/xrRender/DrawUtils.h"
#include "../../Include/xrRender/FontRender.h"
#include "../../Include/xrRender/UIShader.h"
#include "../../Include/xrRender/UIRender.h"
#include "../../Include/xrRender/DebugRender.h"

#define REGISTER(name,byte,size,a1,a2) name=byte,
enum D3DVertexState
{
#include "D3D9VertexState.h"
};
#undef REGISTER




#include "Engine/XRayRenderFactory.h"
#include "Engine/XRayRenderInterface.h"
#include "Engine/XRayUIRender.h"
#include "Engine/XRayDUInterface.h"
#include "Engine/XRayDebugRender.h"


#include "Engine/Factory/XRayConsoleRender.h"
#include "Engine/Factory/XRayEnvDescriptorMixerRender.h"
#include "Engine/Factory/XRayEnvDescriptorRender.h"
#include "Engine/Factory/XRayEnvironmentRender.h"
#include "Engine/Factory/XRayFlareRender.h"
#include "Engine/Factory/XRayFlareRender.h"
#include "Engine/Factory/XRayFontRender.h"
#include "Engine/Factory/XRayLensFlareRender.h"
#include "Engine/Factory/XRayObjectSpaceRender.h"
#include "Engine/Factory/XRayRainRender.h"
#include "Engine/Factory/XRayRenderDeviceRender.h"
#include "Engine/Factory/XRayStatGraphRender.h"
#include "Engine/Factory/XRayStatsRender.h"
#include "Engine/Factory/XRayThunderboltDescRender.h"
#include "Engine/Factory/XRayThunderboltRender.h"
#include "Engine/Factory/XRayUISequenceVideoItem.h"
#include "Engine/Factory/XRayUIShader.h"
#include "Engine/Factory/XRayWallMarkArray.h"


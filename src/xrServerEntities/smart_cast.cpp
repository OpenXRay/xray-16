////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cast.cpp
//	Created 	: 17.09.2004
//  Modified 	: 17.09.2004
//	Author		: Dmitriy Iassenev
//	Description : Smart dynamic cast
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#ifndef PURE_DYNAMIC_CAST
#ifdef XRGAME_EXPORTS
#include "Include/xrRender/rendervisual.h"
#include "alife_space.h"
#include "hit.h"
#include "actor.h"
#include "custommonster.h"
#include "ai/stalker/ai_stalker.h"
#include "xrUICore/Windows/UIWindow.h"
#include "customzone.h"
#include "weapon.h"
#include "cameraeffector.h"
#endif
#include "xrServer_Objects_ALife_Monsters.h"
#endif

#undef SMART_CAST_H
#include "smart_cast.h"

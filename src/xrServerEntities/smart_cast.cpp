////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cast.cpp
//	Created 	: 17.09.2004
//  Modified 	: 17.09.2004
//	Author		: Dmitriy Iassenev
//	Description : Smart dynamic cast
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifndef PURE_DYNAMIC_CAST
#ifdef XRGAME_EXPORTS
#include "Include/xrRender/rendervisual.h"
#include "actor.h"
#include "ai/stalker/ai_stalker.h"
#include "alife_space.h"
#include "cameraeffector.h"
#include "custommonster.h"
#include "customzone.h"
#include "hit.h"
#include "ui/uiwindow.h"
#include "weapon.h"
#endif
#include "xrServer_Objects_ALife_Monsters.h"
#endif

#undef SMART_CAST_H
#include "smart_cast.h"

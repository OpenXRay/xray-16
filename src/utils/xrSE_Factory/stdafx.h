////////////////////////////////////////////////////////////////////////////
//	Module 		: stdafx.h
//	Created 	: 18.06.2004
//  Modified 	: 18.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Precompiled header creator
////////////////////////////////////////////////////////////////////////////

#pragma once

// Hack to Include\xrRender\DrawUtils.h (used and defined in editors)
#define ECORE_API

#include "Common/Common.hpp"
#include "xrCore/xrCore.h"
#include "xrCore/_fbox.h"
#include "xrCore/_quaternion.h"
#include "xrScriptEngine/DebugMacros.hpp" // XXX: move debug macros to xrCore
#include "xrServerEntities/smart_cast.h"

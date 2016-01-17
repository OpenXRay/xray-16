#pragma once

#include "Common/Platform.hpp"

#ifdef XRPHYSICS_EXPORTS
#define XRPHYSICS_API XR_EXPORT
#else
#define XRPHYSICS_API XR_IMPORT
	#ifndef	_EDITOR
		#pragma comment( lib, "xrPhysics.lib"	)
	#else
		#pragma comment( lib, "xrPhysicsB.lib"	)
	#endif
#endif


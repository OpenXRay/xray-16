#pragma once

#ifdef XRPHYSICS_EXPORTS
#define XRPHYSICS_API __declspec(dllexport)
#else
#define XRPHYSICS_API __declspec(dllimport)
	#ifndef	_EDITOR
		#pragma comment( lib, "xrPhysics.lib"	)
	#else
		#pragma comment( lib, "xrPhysicsB.lib"	)
	#endif
#endif


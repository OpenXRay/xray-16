////////////////////////////////////////////////////////////////////////////
//	Module 		: config.h
//	Created 	: 28.10.2006
//  Modified 	: 14.04.2007
//	Author		: Dmitriy Iassenev
//	Description : configuration file
////////////////////////////////////////////////////////////////////////////

#ifndef CS_CONFIG_H_INCLUDED
#define CS_CONFIG_H_INCLUDED

#include <cs/defines.h>

#if defined(WIN32)
#	define CS_PLATFORM_WINDOWS_32
#	define CS_PLATFORM_ID
#elif defined(_XBOX) // #if defined(WIN32)
#	define CS_PLATFORM_XBOX_360
#	define CS_STATIC_LIBRARIES
#	define CS_PLATFORM_ID				[xbox_360]
#else // #elif defined(_XBOX)
	STATIC_CHECK(false, Unknown_Platform);
#endif // #elif defined(_XBOX)

#ifdef DEBUG
#	define CS_DEBUG_LIBRARIES
#	define CS_SOLUTION_CONFIGURATION_ID	(debug)
#else // #ifdef DEBUG
#	define CS_SOLUTION_CONFIGURATION_ID
#endif // #ifdef DEBUG

#define CS_LIBRARY_PREFIX				cs.
#define CS_CALL							__stdcall
#define CS_PACK_SIZE					4

#ifndef CS_API
#	ifdef CS_STATIC_LIBRARIES
#		define CS_API
#	else // #ifdef CS_STATIC_LIBRARIES
#		define CS_API					__declspec(dllimport)
#	endif // #ifdef CS_STATIC_LIBRARIES
#endif // #ifndef CS_API

// exceptions haven't been supported yet
// #define CS_USE_EXCEPTIONS

#endif // #ifndef CS_CONFIG_H_INCLUDED
// Engine_impexp.h
#pragma once

#ifndef ENGINE_API
#ifndef NO_ENGINE_API
#ifdef ENGINE_BUILD
#define DLL_API XR_IMPORT
#define ENGINE_API XR_EXPORT
#else
#undef DLL_API
#define DLL_API XR_EXPORT
#define ENGINE_API XR_IMPORT
#endif
#else
#define ENGINE_API
#define DLL_API
#endif // !NO_ENGINE_API
#endif // !ENGINE_API

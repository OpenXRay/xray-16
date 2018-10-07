///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains preprocessor stuff. This should be the first included header.
 *	\file		IcePreprocessor.h
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __ICEPREPROCESSOR_H__
#define __ICEPREPROCESSOR_H__

// Check compiler options
#ifdef COMPILER_VISUAL_CPP
#if defined(_CHAR_UNSIGNED)
#endif

#if defined(_CPPRTTI)
// #error Please disable RTTI...
#endif

#if defined(_CPPUNWIND)
// #error Please disable exceptions...
#endif

#if defined(_MT)
// Multithreading
#endif
#endif

// Check debug mode
#ifdef DEBUG // May be defined instead of _DEBUG. Let's fix it.
#undef _DEBUG
#define _DEBUG
#endif

#ifdef _DEBUG
// Here you may define items for debug builds
#endif

#ifndef THIS_FILE
#define THIS_FILE __FILE__
#endif
/*
    #ifdef ICECORE_EXPORTS
        #define ICECORE_API			__declspec(dllexport)
    #else
        #define ICECORE_API			__declspec(dllimport)
    #endif
*/
#define FUNCTION extern "C"

// Cosmetic stuff [mainly useful with multiple inheritance]
#define override(baseclass) virtual

// Our own inline keyword, so that:
// - we can switch to __forceinline to check it's really better or not
// - we can remove __forceinline if the compiler doesn't support it
#define inline_ inline

// Down the hatch
#pragma inline_depth(255)

#endif // __ICEPREPROCESSOR_H__

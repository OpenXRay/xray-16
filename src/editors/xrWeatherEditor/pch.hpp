////////////////////////////////////////////////////////////////////////////
//	Module 		: pch.hpp
//	Created 	: 04.12.2007
//  Modified 	: 04.12.2007
//	Author		: Dmitriy Iassenev
//	Description : precompiled header for editor library
////////////////////////////////////////////////////////////////////////////

#pragma once

typedef unsigned int u32;
typedef char const* LPCSTR;
typedef char* LPSTR;

#pragma managed(push, off)
#include <malloc.h>
#pragma managed(pop)

#include <stdlib.h>
#include <vcclr.h>

#pragma warning(disable : 4127)
#pragma warning(disable : 4100)

#pragma managed(push, off)
#include "Common/Common.hpp"
#pragma managed(pop)

#include "xrCore/xrCore.h"
#include "xrEngine/Engine.h"
#include "xrEngine/device.h"
#include "xrSound/Sound.h"

#ifdef DEBUG
#define VERIFY(expression) \
    do                     \
    {                      \
        if (!(expression)) \
            throw;         \
    } while (0)
#define NODEFAULT       \
    do                  \
    {                   \
        DEBUG_BREAK;    \
    } while (0)
#else // #ifdef DEBUG
#define VERIFY(expression) \
    do                     \
    {                      \
    } while (0)
#define NODEFAULT XR_ASSUME(0)
#endif // #ifdef DEBUG

// do not forget to call
// 'cs_free'
// on the block of memory being returned
inline LPSTR to_string(System::String ^ string)
{
    // Pin memory so GC can't move it while native function is called
    pin_ptr<const wchar_t> wch = PtrToStringChars(string);

    size_t convertedChars = 0;
    size_t sizeInBytes = ((string->Length + 1) * 2);
    errno_t err = 0;
    LPSTR result = (LPSTR)malloc(sizeInBytes);

    err = wcstombs_s(&convertedChars, result, sizeInBytes, wch, sizeInBytes);

    if (err)
        VERIFY(!"[tostring][failed] : wcstombs_s failed");

    return (result);
}

inline System::String ^ to_string(LPCSTR string) { return (gcnew System::String(string)); }

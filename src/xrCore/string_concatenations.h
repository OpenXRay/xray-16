#pragma once

#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
#define EXCEPTION_STACK_OVERFLOW ((u32) 0xC00000FD)
#define EXCEPTION_EXECUTE_HANDLER 1
#define EXCEPTION_CONTINUE_SEARCH 0

int _cdecl XRCORE_API _resetstkoflw(void);
#endif

template <typename... Args>
pstr strconcat(size_t size, pstr outStr, const Args... args)
{
    pstr currSymbol = &outStr[0];
    const pstr endSymbol = &outStr[0] + size - 1;

#ifdef MASTER_GOLD
    bool shouldStop = false;
#endif
    std::initializer_list<pcstr> strArgs = { args... };
    for (pcstr strCursor : strArgs)
    {
        while (*strCursor)
        {
#ifdef MASTER_GOLD
            // silently skip and prevent crash
            if (currSymbol == endSymbol)
            {
                shouldStop = true;
            }
#else
            R_ASSERT3(currSymbol != endSymbol, "buffer overflow: cannot concatenate string ", &outStr[0]);
#endif

            *currSymbol = *strCursor;
            currSymbol++;
            strCursor++;
#ifdef MASTER_GOLD
            if (shouldStop)
                break;
#endif
        }
#ifdef MASTER_GOLD
        if (shouldStop)
            break;
#endif
    }

    *currSymbol = '\0';
    return &outStr[0];
}

template <size_t Size, typename... Args>
pstr strconcat(char (&outStr)[Size], const Args... args)
{
    strconcat(Size, &outStr[0], args...);
    return &outStr[0];
}

// warning: do not comment this macro, as stack overflow check is very light
// (consumes ~1% performance of STRCONCAT macro)
#define STRCONCAT_STACKOVERFLOW_CHECK
#ifdef STRCONCAT_STACKOVERFLOW_CHECK

#define STRCONCAT(dest, ...)                                                \
    do                                                                      \
    {                                                                       \
        xray::core::detail::string_tupples tupplesUniqueId(__VA_ARGS__);    \
        size_t bufferSize = tupplesUniqueId.size();                         \
        xray::core::detail::check_stack_overflow(bufferSize);               \
        (dest) = static_cast<pstr>(xr_alloca(bufferSize));                  \
        tupplesUniqueId.concat(dest);                                       \
    } while (0)

#else //#ifdef STRCONCAT_STACKOVERFLOW_CHECK

#define STRCONCAT(dest, ...)                                                \
    do                                                                      \
    {                                                                       \
        xray::core::detail::string_tupples tupplesUniqueId(__VA_ARGS__);    \
        (dest) = static_cast<pstr>(xr_alloca(tupplesUniqueId.size()));      \
        tupplesUniqueId.concat(dest);                                       \
    } while (0)

#endif //#ifdef STRCONCAT_STACKOVERFLOW_CHECK

#include "string_concatenations_inline.h"


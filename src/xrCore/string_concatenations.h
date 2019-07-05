#pragma once

#if defined(LINUX) || defined(FREEBSD)
#define EXCEPTION_STACK_OVERFLOW ((DWORD)0xC00000FD)
#define EXCEPTION_EXECUTE_HANDLER 1
#define EXCEPTION_CONTINUE_SEARCH 0

int _cdecl XRCORE_API _resetstkoflw(void);
#endif

template <typename... Args>
pstr strconcat(u32 size, pstr outStr, const Args... args)
{
    pstr currSymbol = &outStr[0];
    pstr endSymbol = &outStr[0] + size - 1;

    std::initializer_list<pcstr> strArgs = {args...};
    for (pcstr strCursor : strArgs)
    {
        while (*strCursor)
        {
            R_ASSERT3(currSymbol < endSymbol, "buffer overflow: cannot concatenate string ", &outStr[0]);

            *currSymbol = *strCursor;
            currSymbol++;
            strCursor++;
        }
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

#define STRCONCAT(dest, ...)                                    \
    do                                                          \
    {                                                           \
        Strconcat::CStringTupples tupplesUniqueId(__VA_ARGS__); \
        u32 bufferSize = tupplesUniqueId.size();                \
        Strconcat::check_stack_overflow(bufferSize);            \
        (dest) = static_cast<pstr>(_alloca(bufferSize));        \
        tupplesUniqueId.concat(dest);                           \
    } while (0)

#else //#ifdef STRCONCAT_STACKOVERFLOW_CHECK

#define STRCONCAT(dest, ...)                                         \
    do                                                               \
    {                                                                \
        Strconcat::CStringTupples tupplesUniqueId(__VA_ARGS__);      \
        (dest) = static_cast<pstr>(_alloca(tupplesUniqueId.size())); \
        tupplesUniqueId.concat(dest);                                \
    } while (0)

#endif //#ifdef STRCONCAT_STACKOVERFLOW_CHECK

#include "string_concatenations_inline.h"

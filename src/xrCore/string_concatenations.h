#ifndef STRING_CONCATENATIONS_H
#define STRING_CONCATENATIONS_H

#if defined(LINUX) || defined(FREEBSD)
#define EXCEPTION_STACK_OVERFLOW	((DWORD) 0xC00000FD)
#define EXCEPTION_EXECUTE_HANDLER	1
#define EXCEPTION_CONTINUE_SEARCH	0

int _cdecl XRCORE_API _resetstkoflw(void);
#endif

#if 1//ndef _EDITOR

pstr XRCORE_API strconcat(int dest_sz, pstr dest, pcstr S1, pcstr S2);
pstr XRCORE_API strconcat(int dest_sz, pstr dest, pcstr S1, pcstr S2, pcstr S3);
pstr XRCORE_API strconcat(int dest_sz, pstr dest, pcstr S1, pcstr S2, pcstr S3, pcstr S4);
pstr XRCORE_API strconcat(
    int dest_sz, pstr dest, pcstr S1, pcstr S2, pcstr S3, pcstr S4, pcstr S5);
pstr XRCORE_API strconcat(int dest_sz, pstr dest, pcstr S1, pcstr S2, pcstr S3, pcstr S4,
    pcstr S5, pcstr S6);

#else // _EDITOR
// obsolete: should be deleted as soon borland work correctly with new strconcats
IC pstr strconcat(int dest_sz, pstr dest, pcstr S1, pcstr S2)
{
    return xr_strcat(xr_strcpy(dest, dest_sz, S1), dest_sz, S2);
}

// dest = S1+S2+S3
IC pstr strconcat(int dest_sz, pstr dest, pcstr S1, pcstr S2, pcstr S3)
{
    return xr_strcat(xr_strcat(xr_strcpy(dest, dest_sz, S1), dest_sz, S2), dest_sz, S3);
}

// dest = S1+S2+S3+S4
IC pstr strconcat(int dest_sz, pstr dest, pcstr S1, pcstr S2, pcstr S3, pcstr S4)
{
    return xr_strcat(xr_strcat(xr_strcat(xr_strcpy(dest, dest_sz, S1), dest_sz, S2), dest_sz, S3), dest_sz, S4);
}

// dest = S1+S2+S3+S4+S5
IC pstr strconcat(
    int dest_sz, pstr dest, pcstr S1, pcstr S2, pcstr S3, pcstr S4, pcstr S5)
{
    return xr_strcat(
        xr_strcat(xr_strcat(xr_strcat(xr_strcpy(dest, dest_sz, S1), dest_sz, S2), dest_sz, S3), dest_sz, S4), dest_sz,
        S5);
}

// dest = S1+S2+S3+S4+S5+S6
IC pstr strconcat(int dest_sz, pstr dest, pcstr S1, pcstr S2, pcstr S3, pcstr S4,
    pcstr S5, pcstr S6)
{
    return xr_strcat(
        xr_strcat(xr_strcat(xr_strcat(xr_strcat(xr_strcpy(dest, dest_sz, S1), dest_sz, S2), dest_sz, S3), dest_sz, S4),
            dest_sz, S5),
        dest_sz, S6);
}

#endif

// warning: do not comment this macro, as stack overflow check is very light
// (consumes ~1% performance of STRCONCAT macro)
#ifndef _EDITOR
#define STRCONCAT_STACKOVERFLOW_CHECK

#ifdef STRCONCAT_STACKOVERFLOW_CHECK

#define STRCONCAT(dest, ...)                                                                 \
    do                                                                                       \
    {                                                                                        \
        xray::core::detail::string_tupples STRCONCAT_tupples_unique_identifier(__VA_ARGS__); \
        u32 STRCONCAT_buffer_size = STRCONCAT_tupples_unique_identifier.size();              \
        xray::core::detail::check_stack_overflow(STRCONCAT_buffer_size);                     \
        (dest) = (pstr)_alloca(STRCONCAT_buffer_size);                                      \
        STRCONCAT_tupples_unique_identifier.concat(dest);                                    \
    } while (0)

#else //#ifdef STRCONCAT_STACKOVERFLOW_CHECK

#define STRCONCAT(dest, ...)                                                                 \
    do                                                                                       \
    {                                                                                        \
        xray::core::detail::string_tupples STRCONCAT_tupples_unique_identifier(__VA_ARGS__); \
        (dest) = (pstr)_alloca(STRCONCAT_tupples_unique_identifier.size());                 \
        STRCONCAT_tupples_unique_identifier.concat(dest);                                    \
    } while (0)

#endif //#ifdef STRCONCAT_STACKOVERFLOW_CHECK

#endif //_EDITOR
#include "string_concatenations_inline.h"

#endif // #ifndef STRING_CONCATENATIONS_H

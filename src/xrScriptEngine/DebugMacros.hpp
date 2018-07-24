#pragma once
//#include "xrCore/xrCore.h"
#include "xrCore/_types.h"
#include "xrCore/xrDebug.h"

#if XRAY_EXCEPTIONS
#define THROW(expr)                                                                           \
    \
do                                                                                     \
    {                                                                                         \
        if (!(expr))                                                                          \
        {                                                                                     \
            string4096 assertionInfo;                                                         \
            xrDebug::GatherInfo(assertionInfo, sizeof(assertionInfo), DEBUG_INFO, #expr, nullptr, nullptr, nullptr); \
            throw assertionInfo;                                                              \
        }                                                                                     \
    \
}                                                                                      \
    while (false)
#define THROW2(expr, msg0)                                                                 \
    \
do                                                                                  \
    {                                                                                      \
        if (!(expr))                                                                       \
        {                                                                                  \
            string4096 assertionInfo;                                                      \
            xrDebug::GatherInfo(assertionInfo, sizeof(assertionInfo), DEBUG_INFO, #expr, msg0, nullptr, nullptr); \
            throw assertionInfo;                                                           \
        }                                                                                  \
    \
}                                                                                   \
    while (false)
#define THROW3(expr, msg0, msg1)                                                        \
    \
do                                                                               \
    {                                                                                   \
        if (!(expr))                                                                    \
        {                                                                               \
            string4096 assertionInfo;                                                   \
            xrDebug::GatherInfo(assertionInfo, sizeof(assertionInfo), DEBUG_INFO, #expr, msg0, msg1, nullptr); \
            throw assertionInfo;                                                        \
        }                                                                               \
    \
}                                                                                \
    while (false)
#else
#define THROW VERIFY
#define THROW2 VERIFY2
#define THROW3 VERIFY3
#endif

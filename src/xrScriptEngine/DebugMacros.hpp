#pragma once
#include "xrCore/xrCore.h"

#if XRAY_EXCEPTIONS
#define	THROW(expr)\
do {\
    if (!(expr))\
    {\
        string4096 assertion_info;\
        ::Debug.gather_info(_TRE(#expr), nullptr, nullptr, nullptr, DEBUG_INFO, assertion_info);\
        throw assertion_info;\
    }\
} while(0)
#define	THROW2(expr, msg0)\
do {\
    if (!(expr))\
    {\
        string4096 assertion_info;\
        ::Debug.gather_info(_TRE(#expr), msg0, nullptr, nullptr, DEBUG_INFO, assertion_info);\
        throw assertion_info;\
    }\
} while(0)
#define	THROW3(expr, msg0, msg1)\
do {\
    if (!(expr))\
    {\
        string4096 assertion_info;\
        ::Debug.gather_info(_TRE(#expr), msg0, msg1, nullptr, DEBUG_INFO, assertion_info);\
        throw assertion_info;\
    }\
} while(0)
#else
#define	THROW VERIFY
#define	THROW2 VERIFY2
#define	THROW3 VERIFY3
#endif

#ifndef XRSE_FACTORY_IMPORT_EXPORTH
#define XRSE_FACTORY_IMPORT_EXPORTH

#include "Common/Common.hpp"

#ifdef XRAY_STATIC_BUILD
#   define FACTORY_API
#else
#   ifdef XRSE_FACTORY_EXPORTS
#       define FACTORY_API XR_EXPORT
#   else
#       define FACTORY_API XR_IMPORT
#   endif
#endif

extern "C" {
FACTORY_API IServerEntity* __stdcall create_entity(LPCSTR section);
FACTORY_API void __stdcall destroy_entity(IServerEntity*&);
};

#endif

#pragma once
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

namespace xrSE_Factory
{
FACTORY_API IServerEntity* create_entity(LPCSTR section);
FACTORY_API void destroy_entity(IServerEntity*&);
FACTORY_API void initialize();
FACTORY_API void destroy();
} // namespace XrSE_Factory


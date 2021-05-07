#pragma once

#ifdef XRAY_STATIC_BUILD
#   define XRPHYSICS_API
#else
#   ifdef XRPHYSICS_EXPORTS
#      define XRPHYSICS_API XR_EXPORT
#   else
#      define XRPHYSICS_API XR_IMPORT
#   endif
#endif

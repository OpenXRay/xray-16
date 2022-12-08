#pragma once

// XXX: temporary use MASTER_GOLD here, switch to XRAY_STATIC_BUILD later
#ifdef MASTER_GOLD // XRAY_STATIC_BUILD
#   define XRPHYSICS_API
#else
#   ifdef XRPHYSICS_EXPORTS
#      define XRPHYSICS_API XR_EXPORT
#   else
#      define XRPHYSICS_API XR_IMPORT
#   endif
#endif

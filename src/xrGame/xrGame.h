#include "Common/Common.hpp"

class IFactoryObject;

#ifdef XR_PLATFORM_SWITCH
void xrGame_GlobalInit();
#endif

#ifndef XR_PLATFORM_SWITCH
extern "C"
{
#endif
XR_EXPORT IFactoryObject* __cdecl xrFactory_Create(CLASS_ID clsid);
XR_EXPORT void __cdecl xrFactory_Destroy(IFactoryObject* O);
XR_EXPORT void initialize_library();
XR_EXPORT void finalize_library();
#ifndef XR_PLATFORM_SWITCH
}
#endif
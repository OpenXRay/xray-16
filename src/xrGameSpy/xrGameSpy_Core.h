#ifndef XRGAMESPY_CORE
#define XRGAMESPY_CORE

#include "xrGameSpy_MainDefs.h"
#include "GameSpy/common/gsCore.h"

extern "C"
{

EXPORT_FN_DECL(void,	gsCoreInitialize,	());
EXPORT_FN_DECL(void,	gsCoreThink,		(gsi_time theMs));
EXPORT_FN_DECL(void,	gsCoreShutdown,		());

} //extern "C"

#endif
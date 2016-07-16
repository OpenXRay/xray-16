#pragma once
#ifndef XRCORE_IMPEXP_H
#define XRCORE_IMPEXP_H
#include "Common/xr_impexp_macros.h"

#ifdef XRCORE_EXPORTS
#define XRCORE_API XR_EXPORT
#else
#define XRCORE_API XR_IMPORT
#endif

#endif

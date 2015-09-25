#pragma once
#include "xrCore/xrCore.h"
#pragma comment(lib,"xrCore.lib")

#ifdef XRUTIL_EXPORTS
#define XRUTIL_API __declspec(dllexport)
#else
#define XRUTIL_API __declspec(dllimport)
#endif

#pragma once
#include "xrCore/xrCore.h"
#pragma comment(lib, "xrCore.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comctl32.lib")

#ifdef XRLCUTIL_EXPORTS
#define XRLCUTIL_API __declspec(dllexport)
#else
#define XRLCUTIL_API __declspec(dllimport)
#endif

XRLCUTIL_API std::string make_time(u32 sec);

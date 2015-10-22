#pragma once

#ifdef XRAICORE_EXPORTS
#define XRAICORE_API __declspec(dllexport)
#else
#define XRAICORE_API __declspec(dllimport)
#endif

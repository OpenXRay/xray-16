#pragma once

#ifdef XRSCRIPTENGINE_EXPORTS
#define XRSCRIPTENGINE_API __declspec(dllexport)
#else
#define XRSCRIPTENGINE_API __declspec(dllimport)
#endif

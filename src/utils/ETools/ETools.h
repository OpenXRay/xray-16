#pragma once

#ifdef ETOOLS_EXPORTS
#define ETOOLS_API __declspec(dllexport)
#else
#define ETOOLS_API __declspec(dllimport)
#endif

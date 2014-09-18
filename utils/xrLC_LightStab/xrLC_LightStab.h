#pragma once

#ifdef XRLC_LIGHT_STAB_EXPORTS
#	define XRLC_LIGHT_STUB_API __declspec(dllexport)
#else
#	define XRLC_LIGHT_STUB_API __declspec(dllimport)
#endif


#	define XRLC_LIGHT_API __declspec(dllimport)
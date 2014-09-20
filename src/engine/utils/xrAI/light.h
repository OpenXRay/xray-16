#ifndef _XR_LIGHT_
#define _XR_LIGHT_
#pragma once

#include "_d3d_extensions.h"

#pragma pack(push,4)
class xrLIGHT       : public Flight
{
public:
	u32				dwFrame;
	u32				dwController;
};
#pragma pack(pop)

#endif

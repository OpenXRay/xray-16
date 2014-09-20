#ifndef RainRender_included
#define RainRender_included
#pragma once

class CEffect_Rain;
//struct Fsphere;

#include "../../xrCore/_sphere.h"

class IRainRender
{
public:
	virtual ~IRainRender() {;}
	virtual void Copy(IRainRender &_in) = 0;

	virtual void Render(CEffect_Rain &owner) = 0;

	virtual const Fsphere& GetDropBounds() const = 0;
};

#endif	//	RainRender_included
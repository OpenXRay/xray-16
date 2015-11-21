#pragma once

#include "..\..\Include\xrRender\ThunderboltRender.h"

class glThunderboltRender :
	public IThunderboltRender
{
public:
	glThunderboltRender();
	virtual ~glThunderboltRender();

	virtual void Copy(IThunderboltRender &_in);

	virtual void Render(CEffect_Thunderbolt &owner);
private:
	ref_geom			  		hGeom_model;
	ref_geom			  		hGeom_gradient;
};

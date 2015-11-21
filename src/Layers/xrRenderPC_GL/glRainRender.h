#pragma once

#include "../../include/xrRender/RainRender.h"

class glRainRender :
	public IRainRender
{
public:
	glRainRender();
	virtual ~glRainRender();
	virtual void Copy(IRainRender &_in);

	virtual void Render(CEffect_Rain &owner);

	virtual const Fsphere& GetDropBounds() const;

private:
	// Visualization	(rain)
	ref_shader						SH_Rain;
	ref_geom						hGeom_Rain;

	// Visualization	(drops)
	IRender_DetailModel*			DM_Drop;
	ref_geom						hGeom_Drops;
};


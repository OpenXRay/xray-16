#pragma once

#include "..\..\Include\xrRender\FontRender.h"

class glFontRender :
	public IFontRender
{
public:
	glFontRender();
	virtual ~glFontRender();

	virtual void Initialize(LPCSTR cShader, LPCSTR cTexture);
	virtual void OnRender(CGameFont &owner);

private:
	ref_shader				pShader;
	ref_geom				pGeom;
};

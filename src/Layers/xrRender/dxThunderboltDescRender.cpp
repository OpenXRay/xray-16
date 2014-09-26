#include "stdafx.h"
#include "dxThunderboltDescRender.h"

void dxThunderboltDescRender::Copy(IThunderboltDescRender&_in)
{
	*this = *((dxThunderboltDescRender*)&_in);
}

void dxThunderboltDescRender::CreateModel(LPCSTR m_name)
{
	IReader* F			= 0;
	F					= FS.r_open("$game_meshes$",m_name); R_ASSERT2(F,"Empty 'lightning_model'.");
	l_model				= ::RImplementation.model_CreateDM(F);
	FS.r_close			(F);
}

void dxThunderboltDescRender::DestroyModel()
{
	::RImplementation.model_Delete(l_model);
}
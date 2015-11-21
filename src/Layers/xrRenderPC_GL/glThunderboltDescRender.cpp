#include "stdafx.h"
#include "glThunderboltDescRender.h"

void glThunderboltDescRender::Copy(IThunderboltDescRender&_in)
{
	*this = *((glThunderboltDescRender*)&_in);
}

void glThunderboltDescRender::CreateModel(LPCSTR m_name)
{
	IReader* F = 0;
	F = FS.r_open("$game_meshes$", m_name); R_ASSERT2(F, "Empty 'lightning_model'.");
	l_model = ::RImplementation.model_CreateDM(F);
	FS.r_close(F);
}

void glThunderboltDescRender::DestroyModel()
{
	::RImplementation.model_Delete(l_model);
}

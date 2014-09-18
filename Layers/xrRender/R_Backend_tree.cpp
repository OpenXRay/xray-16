#include "stdafx.h"
#pragma hdrstop

#include "r_backend_tree.h"

R_tree::R_tree()
{
	unmap();
}

void	R_tree::unmap		()
{
	c_m_xform_v	= 0;
	c_m_xform	= 0;
	c_consts	= 0;
	c_wave		= 0;
	c_wind		= 0;
	c_c_scale	= 0;
	c_c_bias	= 0;
	c_c_sun		= 0;
}

void	R_tree::set_m_xform_v	(Fmatrix& mat)
{
	if (c_m_xform_v) RCache.set_c(c_m_xform_v, mat);
}

void	R_tree::set_m_xform	(Fmatrix& mat)
{
	if (c_m_xform) RCache.set_c(c_m_xform, mat);
}

void	R_tree::set_consts	(float x, float y, float z, float w)
{
	if (c_consts) RCache.set_c(c_consts, x, y, z, w);
}

void	R_tree::set_wave	(Fvector4& vec)
{
	if (c_wave) RCache.set_c(c_wave, vec);
}

void	R_tree::set_wind	(Fvector4& vec)
{
	if (c_wind) RCache.set_c(c_wind, vec);
}

void	R_tree::set_c_scale	(float x, float y, float z, float w)
{
	if (c_c_scale) RCache.set_c(c_c_scale, x, y, z, w);
}

void	R_tree::set_c_bias	(float x, float y, float z, float w)
{
	if (c_c_bias) RCache.set_c(c_c_bias, x, y, z, w);
}

void	R_tree::set_c_sun	(float x, float y, float z, float w)
{
	if (c_c_sun) RCache.set_c(c_c_sun, x, y, z, w);
}

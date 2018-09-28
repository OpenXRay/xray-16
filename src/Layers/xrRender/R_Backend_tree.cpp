#include "stdafx.h"
#pragma hdrstop

#include "R_Backend_tree.h"

R_tree::R_tree() { unmap(); }
void R_tree::unmap()
{
    c_m_xform_v = nullptr;
    c_m_xform = nullptr;
    c_consts = nullptr;
    c_wave = nullptr;
    c_wind = nullptr;
    c_c_scale = nullptr;
    c_c_bias = nullptr;
    c_c_sun = nullptr;
}

void R_tree::set_m_xform_v(Fmatrix& mat)
{
    if (c_m_xform_v)
        RCache.set_c(c_m_xform_v, mat);
}

void R_tree::set_m_xform(Fmatrix& mat)
{
    if (c_m_xform)
        RCache.set_c(c_m_xform, mat);
}

void R_tree::set_consts(float x, float y, float z, float w)
{
    if (c_consts)
        RCache.set_c(c_consts, x, y, z, w);
}

void R_tree::set_wave(Fvector4& vec)
{
    if (c_wave)
        RCache.set_c(c_wave, vec);
}

void R_tree::set_wind(Fvector4& vec)
{
    if (c_wind)
        RCache.set_c(c_wind, vec);
}

void R_tree::set_c_scale(float x, float y, float z, float w)
{
    if (c_c_scale)
        RCache.set_c(c_c_scale, x, y, z, w);
}

void R_tree::set_c_bias(float x, float y, float z, float w)
{
    if (c_c_bias)
        RCache.set_c(c_c_bias, x, y, z, w);
}

void R_tree::set_c_sun(float x, float y, float z, float w)
{
    if (c_c_sun)
        RCache.set_c(c_c_sun, x, y, z, w);
}

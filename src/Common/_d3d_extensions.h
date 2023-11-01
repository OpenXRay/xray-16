#pragma once
#ifndef _D3D_EXT_internal
#define _D3D_EXT_internal
#include "xrCore/FixedVector.h"

#ifndef NO_XR_LIGHT
struct Flight
{
    enum class Type : u32
    {
        Point       = 1,
        Spot        = 2,
        Directional = 3,
    };

public:
    Type type; /* Type of light source */
    Fcolor diffuse; /* Diffuse color of light */
    Fcolor specular; /* Specular color of light */
    Fcolor ambient; /* Ambient color of light */
    Fvector position; /* Position in world space */
    Fvector direction; /* Direction in world space */
    float range; /* Cutoff range */
    float falloff; /* Falloff */
    float attenuation0; /* Constant attenuation */
    float attenuation1; /* Linear attenuation */
    float attenuation2; /* Quadratic attenuation */
    float theta; /* Inner angle of spotlight cone */
    float phi; /* Outer angle of spotlight cone */

    IC void set(Type ltType, float x, float y, float z)
    {
        ZeroMemory(this, sizeof(Flight));
        type = ltType;
        diffuse.set(1.0f, 1.0f, 1.0f, 1.0f);
        specular.set(diffuse);
        position.set(x, y, z);
        direction.set(x, y, z);
        direction.normalize_safe();
        range = _sqrt(flt_max);
    }
    IC void mul(float brightness)
    {
        diffuse.mul_rgb(brightness);
        ambient.mul_rgb(brightness);
        specular.mul_rgb(brightness);
    }
};

#   ifdef _d3d9TYPES_H_
static_assert(sizeof(Flight::Type) == sizeof(D3DLIGHTTYPE));
static_assert(sizeof(Flight) == sizeof(D3DLIGHT9));
#   else
static_assert(sizeof(Flight::Type) == 4);
static_assert(sizeof(Flight) == 104);
#   endif
#endif // !NO_XR_LIGHT

#ifndef NO_XR_MATERIAL
struct Fmaterial
{
public:
    Fcolor diffuse; /* Diffuse color RGBA */
    Fcolor ambient; /* Ambient color RGB */
    Fcolor specular; /* Specular 'shininess' */
    Fcolor emissive; /* Emissive color RGB */
    float power; /* Sharpness if specular highlight */

    IC void set(float r, float g, float b)
    {
        ZeroMemory(this, sizeof(Fmaterial));
        diffuse.r = ambient.r = r;
        diffuse.g = ambient.g = g;
        diffuse.b = ambient.b = b;
        diffuse.a = ambient.a = 1.0f;
        power = 0;
    }
    IC void set(float r, float g, float b, float a)
    {
        ZeroMemory(this, sizeof(Fmaterial));
        diffuse.r = ambient.r = r;
        diffuse.g = ambient.g = g;
        diffuse.b = ambient.b = b;
        diffuse.a = ambient.a = a;
        power = 0;
    }
    IC void set(Fcolor& c)
    {
        ZeroMemory(this, sizeof(Fmaterial));
        diffuse.r = ambient.r = c.r;
        diffuse.g = ambient.g = c.g;
        diffuse.b = ambient.b = c.b;
        diffuse.a = ambient.a = c.a;
        power = 0;
    }
};

#   ifdef _d3d9TYPES_H_
static_assert(sizeof(Fmaterial) == sizeof(D3DMATERIAL9));
#   else
static_assert(sizeof(Fmaterial) == 68);
#   endif
#endif // !NO_XR_MATERIAL

#endif

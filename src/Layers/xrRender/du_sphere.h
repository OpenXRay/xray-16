#pragma once

#define DU_SPHERE_NUMVERTEX 92
#define DU_SPHERE_NUMFACES 180
#define DU_SPHERE_NUMVERTEXL 60
#define DU_SPHERE_NUMLINES 60

namespace xray::render::RENDER_NAMESPACE
{
extern ECORE_API Fvector du_sphere_vertices[];
extern ECORE_API u16 du_sphere_faces[];
extern ECORE_API Fvector du_sphere_verticesl[];
extern ECORE_API u16 du_sphere_lines[];
} // namespace xray::render::RENDER_NAMESPACE

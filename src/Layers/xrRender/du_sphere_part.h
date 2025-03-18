#pragma once

#define DU_SPHERE_PART_NUMVERTEX 82
#define DU_SPHERE_PART_NUMFACES 160
#define DU_SPHERE_PART_NUMLINES 176

namespace xray::render::RENDER_NAMESPACE
{
extern ECORE_API Fvector du_sphere_part_vertices[];
extern ECORE_API u16 du_sphere_part_faces[];
extern ECORE_API u16 du_sphere_part_lines[];
} // namespace xray::render::RENDER_NAMESPACE

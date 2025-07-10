#pragma once

#define DU_CYLINDER_NUMVERTEX 26
#define DU_CYLINDER_NUMFACES 48
#define DU_CYLINDER_NUMLINES 30 // 36

namespace xray::render::RENDER_NAMESPACE
{
extern ECORE_API Fvector du_cylinder_vertices[];
extern ECORE_API u16 du_cylinder_faces[];
extern ECORE_API u16 du_cylinder_lines[];
} // namespace xray::render::RENDER_NAMESPACE

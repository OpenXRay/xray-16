#pragma once

#define DU_BOX_NUMVERTEX 8
#define DU_BOX_NUMFACES 12
#define DU_BOX_NUMLINES 12
#define DU_BOX_NUMVERTEX2 36

namespace xray::render::RENDER_NAMESPACE
{
extern ECORE_API Fvector du_box_vertices[];
extern ECORE_API u16 du_box_faces[];
extern ECORE_API u16 du_box_lines[];

extern ECORE_API Fvector du_box_vertices2[];
} // namespace xray::render::RENDER_NAMESPACE

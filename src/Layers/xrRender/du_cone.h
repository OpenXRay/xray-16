#pragma once

#define DU_CONE_NUMVERTEX 18
#define DU_CONE_NUMFACES 32
#define DU_CONE_NUMLINES 24

namespace xray::render::RENDER_NAMESPACE
{
extern ECORE_API Fvector du_cone_vertices[];
extern ECORE_API u16 du_cone_faces[];
extern ECORE_API u16 du_cone_lines[];
} // namespace xray::render::RENDER_NAMESPACE

#include "stdafx.h"
#include "../xrRender/du_sphere_part.h"

void CRenderTarget::accum_omnip_geom_create		()
{
	GLenum	dwUsage				= GL_STATIC_DRAW;

	// vertices
	{
		u32		vCount		= DU_SPHERE_PART_NUMVERTEX;
		u32		vSize		= 3*4;
		glGenBuffers(1, &g_accum_omnip_vb);
		glBindBuffer(GL_ARRAY_BUFFER, g_accum_omnip_vb);
		CHK_GL(glBufferData(GL_ARRAY_BUFFER, vCount*vSize, du_sphere_part_vertices, dwUsage));
	}

	// Indices
	{
		u32		iCount		= DU_SPHERE_PART_NUMFACES*3;
		glGenBuffers(1, &g_accum_omnip_ib);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_accum_omnip_ib);
		CHK_GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, iCount * 2, du_sphere_part_faces, dwUsage));
	}
}

void CRenderTarget::accum_omnip_geom_destroy()
{
	glDeleteBuffers(1, &g_accum_omnip_vb);
	glDeleteBuffers(1, &g_accum_omnip_ib);
}

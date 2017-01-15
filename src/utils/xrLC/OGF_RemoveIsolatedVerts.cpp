#include "MeshMenderLayerOGF.h"
#include "stdafx.h"

void remove_isolated_verts(vecOGF_V& vertices, vecOGF_F& faces)
{
    t_remove_isolated_verts(vertices, faces);
}
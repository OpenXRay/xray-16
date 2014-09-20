#ifndef __NVMESHMENDERLAYER_H__
#define __NVMESHMENDERLAYER_H__

#include	"NvMender2003/nvMeshMender.h"



IC void	set_vertex	( MeshMender::Vertex &out_vertex, const Vertex& in_veretex, const Fvector2 Ftc );
IC void	set_face	( Face &out_face, const MeshMender::Vertex in_vertices[3] );
#include	"NvMeshMenderLayerInline.h"
#endif //__NVMESHMENDERLAYER_H__
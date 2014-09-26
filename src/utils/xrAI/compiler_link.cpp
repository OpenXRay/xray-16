#include "stdafx.h"
#include "compiler.h"

IC void ProcessID(u32 ID, NodeMerged& N, u32 OurID, Marks& m_used)
{
	if (ID == InvalidNode)	return;
	u32 group = g_nodes[ID].Group;
	if (group==OurID)		return;
	if (m_used[group])		return;

	m_used[group]			= true;
	N.neighbours.push_back	(group);
}

void xrConvertAndLink()
{
	Status("Convertion and Linking...");

	for (u32 it=0; it<g_merged.size(); it++)
	{
		NodeMerged& N = g_merged[it];
		
		// build node
		xr_vector<Fvector>	points;
		N.plane.n.set	(0,0,0);
		float			light=0;
		N.cover[0]		=0;
		N.cover[1]		=0;
		N.cover[2]		=0;
		N.cover[3]		=0;
		for (u32 L=0; L<N.contains.size(); L++) {
			vertex&	element		= g_nodes[N.contains[L]];
			points.push_back	(element.Pos);
			N.plane.n.add		(element.Plane.n);
			N.sector			=  element.Sector;
			light				+= float(element.LightLevel);
			N.cover[0]			+= element.cover[0];
			N.cover[1]			+= element.cover[1];
			N.cover[2]			+= element.cover[2];
			N.cover[3]			+= element.cover[3];
		}
		float size			= float(N.contains.size());
		N.plane.n.div		(size);
		light				/= size;	clamp(light,0.f,1.f);
		N.light				= BYTE(iFloor(light*255.f+0.1f));
		N.cover[0]			/= size;
		N.cover[1]			/= size;
		N.cover[2]			/= size;
		N.cover[3]			/= size;
		
		// align plane on points, calc extents
		Fbox	BB;			BB.invalidate();
		Fvector vOffs,vNorm;
		vNorm.normalize	(N.plane.n);
		vOffs.set		(0,-1000,0);
		N.plane.build	(vOffs,vNorm);
		for (u32 p=0; p<points.size(); p++)
		{
			Fvector&	V = points[p];
			BB.modify	(V);
			float dist	= N.plane.classify(V);
			if (dist>0) {
				vOffs	= points[p];
				N.plane.build(vOffs,vNorm);
			}
		}

		// "project" extents
		{
			Fvector		D;
			D.set		(0,1,0);
			N.plane.intersectRayPoint(BB.min,D,N.P0);
			N.plane.intersectRayPoint(BB.max,D,N.P1);
		}

		// build neibourhoods list
		Marks		m_used;
		m_used.assign(g_merged.size(),false);
		for (L=0; L<N.contains.size(); L++) {
			u32	ID	= N.contains[L];
			vertex&	E	= g_nodes[ID];

			ProcessID	(E.n1,N,it,m_used);
			ProcessID	(E.n2,N,it,m_used);
			ProcessID	(E.n3,N,it,m_used);
			ProcessID	(E.n4,N,it,m_used);
		}

		// indicator
		Progress(float(it)/float(g_merged.size()));
	}
}


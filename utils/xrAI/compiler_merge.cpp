#include "stdafx.h"
#include "compiler.h"

Marks	used;

BOOL	NodeSimilar(vertex&	N1, vertex&	N2)
{
	// sector
	if (N1.Sector!=N2.Sector)										return FALSE;
	
	// lighting
	float l_eps = sim_light/256.f;
	if (!fsimilar(N1.LightLevel,N2.LightLevel,l_eps))				return FALSE;

	// plane
	if (N1.Plane.n.dotproduct(N2.Plane.n)<_cos(deg2rad(sim_angle)))	return FALSE;
	if (N1.Plane.distance(N2.Pos)>sim_dist)							return FALSE;
	if (N2.Plane.distance(N1.Pos)>sim_dist)							return FALSE;

	// cover
	float c_eps = sim_cover/256.f;
	if (!fsimilar(N1.cover[0],N2.cover[0],c_eps))					return FALSE;
	if (!fsimilar(N1.cover[1],N2.cover[1],c_eps))					return FALSE;
	if (!fsimilar(N1.cover[2],N2.cover[2],c_eps))					return FALSE;
	if (!fsimilar(N1.cover[3],N2.cover[3],c_eps))					return FALSE;

	return TRUE;
}

DEF_VECTOR(vecDW,u32);

xr_vector<vecDW>	BestQuad;
u32			BestQuad_Count;

void ProcessOne		(u32 Base, u32 limit=8)
{
	BestQuad.clear	();
	BestQuad_Count	= 0;
	
	// ***** build horizontal line
	vecDW			BaseLine;
	BaseLine.reserve(limit*2);
	u32			BL_Left=0,BL_Right=0;
	
	// middle
	vertex&			BaseNode = g_nodes[Base];
	BaseLine.push_back(Base);
	
	// left expansion
	for (;;) {
		vertex&	B	= g_nodes[BaseLine.front()];
		u32	LP	= B.nLeft();
		
		if (BL_Left>limit)						break;
		if (LP==InvalidNode)					break;
		if (used[LP])							break;
		if (!NodeSimilar(BaseNode,g_nodes[LP]))	break;
		
		BL_Left	++;
		BaseLine.insert(BaseLine.begin(),LP);
	}
	
	// right expansion
	for (;;) {
		vertex&	B	= g_nodes[BaseLine.back()];
		u32	RP	= B.nRight();
		
		if (BL_Right>limit)						break;
		if (RP==InvalidNode)					break;
		if (used[RP])							break;
		if (!NodeSimilar(BaseNode,g_nodes[RP]))	break;
		
		BL_Right++;
		BaseLine.push_back(RP);
	}
	
	// main cycle
	//	Msg("-----");
	u32	BasePos	= BaseLine[BL_Left];
	for (u32 left=0; left<=BL_Left; left++)
	{
		u32	limit_right	= left+limit-1;
		if (limit_right>=BaseLine.size())	limit_right=BaseLine.size()-1;
		u32	limit_left	= left;
		if (limit_left<BL_Left)				limit_left = BL_Left;
		if (limit_left>limit_right)			limit_left = limit_right;
		for (int right=int(limit_right); right>=int(limit_left); right--)
		{
			// now we have range [left,right]
			// expand it up and down
			xr_vector<vecDW>	stack_up;
			xr_vector<vecDW>	stack_down;
			
			//			Msg("[%2d,%2d], %d",	left,right,BaseLine.size());
			
			stack_up.reserve		(limit);
			stack_up.push_back		(vecDW());
			stack_up.back().assign	(BaseLine.begin()+left,BaseLine.begin()+right+1);
			stack_down.reserve		(limit);
			stack_down.push_back	(vecDW());
			stack_down.back().assign(BaseLine.begin()+left,BaseLine.begin()+right+1);
			
			// expand up
			for (;stack_up.size()<=limit;) {
				// create _new list
				stack_up.push_back	(vecDW());
				vecDW&	src			= stack_up[stack_up.size()-2];
				vecDW&	dest		= stack_up[stack_up.size()-1];
				dest.reserve		(limit);
				BOOL				bFailed = FALSE;
				
				// iterate on it
				vecDW_it I			= src.begin	();
				vecDW_it E			= src.end	();
				for (; I!=E; I++)
				{
					u32	id	= g_nodes[*I].nForward();
					if	(id==InvalidNode)					{ bFailed=TRUE; break; }
					if	(used[id])							{ bFailed=TRUE; break; }
					if	(!NodeSimilar(g_nodes[id],BaseNode)){ bFailed=TRUE; break; }
					dest.push_back	(id);
				}
				if (bFailed) {
					stack_up.pop_back();
					break;
				}
			}
			// expand down
			for (; (stack_up.size()+stack_down.size()-1) <= limit;) {
				// create _new list
				stack_down.push_back(vecDW());
				vecDW&	src			= stack_down[stack_down.size()-2];
				vecDW&	dest		= stack_down[stack_down.size()-1];
				dest.reserve		(limit);
				BOOL				bFailed = FALSE;
				
				// iterate on it
				vecDW_it I			= src.begin	();
				vecDW_it E			= src.end	();
				for (; I!=E; I++)
				{
					u32	id	= g_nodes[*I].nBack();
					if	(id==InvalidNode)					{ bFailed=TRUE; break; }
					if	(used[id])							{ bFailed=TRUE; break; }
					if	(!NodeSimilar(g_nodes[id],BaseNode)){ bFailed=TRUE; break; }
					dest.push_back	(id);
				}
				if (bFailed) {
					stack_down.pop_back();
					break;
				}
			}
			
			// calculate size
			u32 size_z	= stack_up.size()+stack_down.size()-1;
			u32 size_x	= stack_up.back().size();
			u32 size_mix	= size_z*size_x;
			if (size_mix>BestQuad_Count)	
			{
				BestQuad_Count		= size_mix;
				BestQuad.clear		();
				BestQuad.reserve	(size_z);
				
				// transfer quad
				for (u32 it=stack_up.size()-1; it>0; it--)
					BestQuad.push_back(stack_up[it]);
				BestQuad.insert(BestQuad.begin(),stack_down.begin(),stack_down.end());
			}
		}
	}
}


BOOL QuadFit(u32 Base, u32 Size)
{
	// ***** build horizontal line
	vecDW			BaseLine;
	BaseLine.reserve(Size);
	
	// middle
	vertex&			BaseNode = g_nodes[Base];
	BaseLine.push_back(Base);
	
	// right expansion
	for (; BaseLine.size()<Size; ) 
	{
		vertex&	B	= g_nodes[BaseLine.back()];
		u32	RP	= B.nRight();
		
		if (RP==InvalidNode)					break;
		if (used[RP])							break;
		if (!NodeSimilar(BaseNode,g_nodes[RP]))	break;
		
		BaseLine.push_back(RP);
	}
	if (BaseLine.size()<Size)	return FALSE;
	
	// down expansion
	BestQuad.clear	();
	BestQuad_Count	= 0;
	BestQuad.reserve		(Size);
	BestQuad.push_back		(BaseLine);
	
	for (; BestQuad.size() < Size;) {
		// create _new list
		BestQuad.push_back	(vecDW());
		vecDW&	src			= BestQuad[BestQuad.size()-2];
		vecDW&	dest		= BestQuad[BestQuad.size()-1];
		dest.reserve		(Size);
		
		// iterate on it
		vecDW_it I			= src.begin	();
		vecDW_it E			= src.end	();
		for (; I!=E; I++)
		{
			u32	id	= g_nodes[*I].nBack();
			if	(id==InvalidNode)						return FALSE;
			if	(used[id])								return FALSE;
			if	(!NodeSimilar(g_nodes[id],BaseNode))	return FALSE;
			dest.push_back	(id);
		}
	}
	return TRUE;
}

void CreatePN(u32& group_id)
{
	// grab results
	Fvector	min,max;
	min.set(flt_max,flt_max,flt_max);
	max.set(flt_min,flt_min,flt_min);

	NodeMerged		NM;
	for (u32 L=0; L<BestQuad.size(); L++)
	{
		vecDW&	vec = BestQuad[L];
		
		for (u32 N=0; N<vec.size(); N++)
		{
			u32 ID			= vec[N];
			used[ID]			= true;
			g_nodes[ID].Group	= group_id;

			NM.contains.push_back(ID);
		}
	}
	g_merged.push_back(NM);

	group_id++;
}

void xrMerge()
{
	// clear marks to "false"
	used.assign	(g_nodes.size(),false);
	//	rmark.assign(g_nodes.size(),false);
	
	// iterate on nodes
	u32 group_id		= 0;
	u32 start_time	= timeGetTime();
	for (u32 Size=16; Size>2; Size/=2)
	{
		Msg("Pass size: %d",Size);
		for (u32 i=0; i<g_nodes.size(); i++)
		{
			if (!used[i]) {
				// analyze
				vertex& Start = g_nodes[i];

				int px,pz;
				px = iFloor(Start.Pos.x/g_params.fPatchSize+EPS_L);
				pz = iFloor(Start.Pos.z/g_params.fPatchSize+EPS_L);

				if (px%Size!=0 || pz%Size!=0) continue;

				if (QuadFit(i,Size)) CreatePN(group_id);
			}
			Progress(float(i)/float(g_nodes.size()));
		}
	}
	for (u32 i=0; i<g_nodes.size(); i++)
	{
		if (!used[i]) {
			// analyze
			ProcessOne	(i,8);
			CreatePN	(group_id);
		}
		Progress(float(i)/float(g_nodes.size()));
	}

	Msg("Optimization ratio: %2.1f%%\n",	100.f*float(group_id)/float(g_nodes.size())	);
	Msg("%d / %d\n%d seconds elapsed.",
		group_id,g_nodes.size(),
		(timeGetTime()-start_time)/1000);
}

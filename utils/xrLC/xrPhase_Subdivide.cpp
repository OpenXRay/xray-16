#include "stdafx.h"
#include "build.h"
#include "../xrlc_light/xrdeflector.h"
#include "../xrlc_light/xrface.h"
#include "../xrlc_light/xrlc_globaldata.h"

extern void		Detach		(vecFace* S);

void	setup_bbs	(Fbox& b1, Fbox& b2, Fbox& bb,int edge)	{
	Fvector	size;
	b1.set	(bb);	b2.set(bb);
	size.sub(bb.max,bb.min);
	switch	(edge)	{
		case 0:
			b1.max.x -= size.x/2;
			b2.min.x += size.x/2;
			break;
		case 1:
			b1.max.y -= size.y/2;
			b2.min.y += size.y/2;
			break;
		case 2:
			b1.max.z -= size.z/2;
			b2.min.z += size.z/2;
			break;
	}
};

void CBuild::xrPhase_Subdivide()
{
	Status	("Subdividing in space...");
	vecFace s1, s2;
	Fbox	b1, b2;
	for (int X=0; X<int(g_XSplit.size()); X++)
	{
		if (g_XSplit[X]->empty()) 
		{
			xr_delete		(g_XSplit[X]);
			g_XSplit.erase	(g_XSplit.begin()+X);
			X--;
			continue;
		}
		Progress			(float(X)/float(g_XSplit.size()));
		
		// skip if subdivision is too small already
		if (int(g_XSplit[X]->size())<(c_SS_LowVertLimit*2))	continue;
		
		// calc bounding box
		Fbox	bb;
		Fvector size;
		
		bb.invalidate();
		for (vecFaceIt F=g_XSplit[X]->begin(); F!=g_XSplit[X]->end(); F++) 
		{
			Face *XF = *F;
			bb.modify(XF->v[0]->P);
			bb.modify(XF->v[1]->P);
			bb.modify(XF->v[2]->P);
		}
		
		// analyze if we need to split
		size.sub(bb.max,bb.min);
		BOOL	bSplit	= FALSE;
		if  	(size.x>c_SS_maxsize)					bSplit	= TRUE;
		if		(size.y>c_SS_maxsize)					bSplit	= TRUE;
		if		(size.z>c_SS_maxsize)					bSplit	= TRUE;
		if		(int(g_XSplit[X]->size()) > c_SS_HighVertLimit)	bSplit	= TRUE;
		CDeflector*	defl_base	= (CDeflector*)g_XSplit[X]->front()->pDeflector;
		if		(!bSplit && defl_base)	{
			if (defl_base->layer.width  >=	(c_LMAP_size-2*BORDER))	bSplit	= TRUE;
			if (defl_base->layer.height >=	(c_LMAP_size-2*BORDER))	bSplit	= TRUE;
		}

		// perform subdivide if needed
		if (!bSplit)	continue;
		
		// select longest BBox edge
		int		box_edge			= -1;	
		if (size.x>=size.y && size.x>=size.z) {
			box_edge	= 0;
		} else {
			if (size.y>=size.x && size.y>=size.z) {
				box_edge	= 1;
			} else {
				box_edge	= 2;
			}
		}
		setup_bbs	(b1,b2,bb,box_edge);

		// align plane onto vertices
		
		// Process all faces and rearrange them
		u32		iteration_on_edge	= 0	;	// up to 3
		u32		iteration_per_edge	= 0	;	// up to 10
resplit:
		s2.clear	();
		s1.clear	();
		iteration_per_edge		++	;
		for (vecFaceIt F=g_XSplit[X]->begin(); F!=g_XSplit[X]->end(); F++) 
		{
			Face *XF = *F;
			Fvector C;
			XF->CalcCenter(C);
			if (b1.contains(C))	{ s1.push_back(XF); }
			else				{ s2.push_back(XF); }
		}
		
		if ((int(s1.size())<c_SS_LowVertLimit) || (int(s2.size())<c_SS_LowVertLimit))
		{
			// splitting failed
			clMsg	("! ERROR: model #%d - split fail, faces: %d, s1/s2:%d/%d",X,g_XSplit[X]->size(),s1.size(),s2.size());
			if (iteration_per_edge<10)	{
				if		(g_XSplit[X]->size() > c_SS_LowVertLimit*4)		
				{
					if (s2.size()>s1.size())	
					{	//b2 -less, b1-grow
						size.sub	(b2.max,b2.min);
						b1.max[box_edge]	+= size[box_edge]/2;
						b2.min[box_edge]	=  b1.max[box_edge];
					} else {
						//b2 -grow, b1-less
						size.sub	(b1.max,b1.min);
						b2.min[box_edge]	-= size[box_edge]/2;
						b1.max[box_edge]	=  b2.min[box_edge];
					}
					goto		resplit;
				}
			} else {
				// switch edge
				iteration_per_edge	= 0	;
				iteration_on_edge	++	;
				if (iteration_on_edge<3)	{
					box_edge		= (box_edge+1)%3;
					setup_bbs		(b1,b2,bb,box_edge);
					goto		resplit;
				}
			}
		} else {
			// split deflector into TWO
			if (defl_base)	
			{
				// _delete old deflector
				for (u32 it=0; it<lc_global_data()->g_deflectors().size(); it++)
				{
					if (lc_global_data()->g_deflectors()[it]==defl_base)	{
						lc_global_data()->g_deflectors().erase	(lc_global_data()->g_deflectors().begin()+it);
						xr_delete			(defl_base);
						break;
					}
				}
				
				// Create _new deflectors
				CDeflector*		D1	= xr_new<CDeflector>(); 
				//Deflector = D1;
				D1->OA_Place(s1); 
				D1->OA_Export();
				lc_global_data()->g_deflectors().push_back(D1);

				
				CDeflector*		D2	= xr_new<CDeflector>(); 
				//Deflector		= D2;
				D2->OA_Place(s2); 
				D2->OA_Export(); 
				lc_global_data()->g_deflectors().push_back(D2);
			}
			
			// Delete old SPLIT and push two new
			xr_delete				(g_XSplit[X]);
			g_XSplit.erase			(g_XSplit.begin()+X); X--;
			g_XSplit.push_back		(xr_new<vecFace>(s1));	Detach(&s1);
			g_XSplit.push_back		(xr_new<vecFace>(s2));	Detach(&s2);
		}
		s1.clear	();
		s2.clear	();
	}
	clMsg("%d subdivisions.",g_XSplit.size());
	validate_splits	();
}

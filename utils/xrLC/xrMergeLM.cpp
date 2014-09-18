#include "stdafx.h"
#include "build.h"
 
IC bool cmp_defl(CDeflector *p1, CDeflector *p2)
{
	Fvector &C = Deflector->Sphere.P;
	u16 M1 = p1->GetBaseMaterial();
	u16 M2 = p2->GetBaseMaterial();
	if (M1<M2) return true;
	if (M1>M2) return false;
	return C.distance_to_sqr(p1->Sphere.P) < C.distance_to_sqr(p1->Sphere.P);
}

typedef xr_vector<_rect>	vecR;
typedef vecR::iterator		vecRIT;

typedef xr_vector<_point>	vecP;
typedef vecP::iterator		vecPIT;

typedef xr_vector<int>		vecI;
typedef vecI::iterator		vecIIT;

// Data in
static	vecR	rects;
static	vecR	selected;

// Data in process
static	vecR	collected;
static	vecI	perturb;

// Result
static	vecR	best;
static	vecI	best_seq;
_rect	brect  = { {0,0}, {0,0}, INT_MAX};
_rect	currect= { {0,0}, {0,0}, 0 };

// Sorting by areas
IC bool cmp_rect(int r1, int r2)
{
	return selected[r1].iArea > selected[r2].iArea;	// Need decreasing order
}

static	BYTE	surface[lmap_size*lmap_size];
static	int		current_rect	= 0;
const	u32	alpha_ref		= 254-BORDER;

// Initialization
void InitSurface()
{
	FillMemory(surface,lmap_size*lmap_size,0);
}

// Rendering of rect
void _rect_register(_rect &R, CDeflector* D, BOOL bRotate)
{
	collected.push_back(R);
	
	LPDWORD lm	= D->lm.pSurface;
	u32	s_x	= D->lm.dwWidth+2*BORDER;
	u32	s_y = D->lm.dwHeight+2*BORDER;
	
	if (!bRotate) {
		// Normal (and fastest way)
		for (u32 y=0; y<s_y; y++)
		{
			BYTE*	P = surface+(y+R.a.y)*lmap_size+R.a.x;	// destination scan-line
			u32*	S = lm + y*s_x;
			for (u32 x=0; x<s_x; x++,P++) 
			{
				u32 C = *S++;
				u32 A = RGBA_GETALPHA	(C);
				if (A>=alpha_ref)	*P	= 255;
			}
		}
	} else {
		// Rotated :(
		for (u32 y=0; y<s_x; y++)
		{
			BYTE*	P = surface+(y+R.a.y)*lmap_size+R.a.x;	// destination scan-line
			for (u32 x=0; x<s_y; x++,P++)
			{
				u32 C = lm[x*s_x+y];
				u32 A = RGBA_GETALPHA(C);
				if (A>=alpha_ref)	*P	= 255;
			}
		}
	}
}

// Test of per-pixel intersection (surface test)
bool Place_Perpixel(_rect& R, CDeflector* D, BOOL bRotate)
{
	LPDWORD lm			= D->lm.pSurface ;
	u32	s_x			= D->lm.dwWidth	+2*BORDER;
	u32	s_y			= D->lm.dwHeight+2*BORDER;
	
	if (!bRotate) {
		// Normal (and fastest way)
		for (u32 y=0; y<s_y; y++)
		{
			BYTE*	P = surface+(y+R.a.y)*lmap_size+R.a.x;	// destination scan-line
			u32*	S = lm + y*s_x;
			for (u32 x=0; x<s_x; x++,P++) 
			{
				u32 C = *S++;
				u32 A = RGBA_GETALPHA(C);
				if ((*P)&&(A>=alpha_ref))	return false;
			}
		}
	} else {
		// Rotated :(
		for (u32 y=0; y<s_x; y++)
		{
			BYTE*	P = surface+(y+R.a.y)*lmap_size+R.a.x;	// destination scan-line
			for (u32 x=0; x<s_y; x++,P++)
			{
				u32 C = lm[x*s_x+y];
				u32 A = RGBA_GETALPHA(C);
				if ((*P)&&(A>=alpha_ref))	return false;
			}
		}
	}
	
	// It's OK to place it
	return true;
}

// Check for intersection
BOOL _rect_place(_rect &r, CDeflector* D)
{
	// Normal
	{
		_rect R;
		u32 x_max = lmap_size-r.b.x; 
		u32 y_max = lmap_size-r.b.y; 
		for (u32 _Y=0; _Y<y_max; _Y++)
		{
			for (u32 _X=0; _X<x_max; _X++)
			{
				if (surface[_Y*lmap_size+_X]) continue;
				R.init(_X,_Y,_X+r.b.x,_Y+r.b.y);
				if (Place_Perpixel(R,D,FALSE)) {
					_rect_register(R,D,FALSE);
					return TRUE;
				}
			}
		}
	}

	// Rotated
	{
		_rect R;
		u32 x_max = lmap_size-r.b.y; 
		u32 y_max = lmap_size-r.b.x; 
		for (u32 _Y=0; _Y<y_max; _Y++)
		{
			for (u32 _X=0; _X<x_max; _X++)
			{
				if (surface[_Y*lmap_size+_X]) continue;

				R.init(_X,_Y,_X+r.b.y,_Y+r.b.x);
				if (Place_Perpixel(R,D,TRUE)) {
					_rect_register(R,D,TRUE);
					return TRUE;
				}
			}
		}
	}

	return FALSE;
};

#pragma optimize( "g", off )
void CBuild::MergeLM()
{
	vecDefl		Layer;
	vecDefl		deflNew;
	vecDefl		SEL;

	Status("Processing...");
	for (u32 light_layer=0; light_layer<pBuild->lights.size(); light_layer++)
	{
		// Select all deflectors, which contain this light-layer
		Layer.clear	();
		b_light*	L_base	= pBuild->lights[light_layer].original;
		for (int it=0; it<(int)g_deflectors.size(); it++)
		{
			if (g_deflectors[it].bMerged)				continue;
			if (0==g_deflectors[it].GetLayer(L_base))	continue;	
			Layer.push_back	(g_deflectors[it]);
		}
		if (Layer.empty())	continue;
		
		// Resort layer


		// Merge this layer
		while (Layer.size()) 
		{
			// Sort layer (by material and distance from "base" deflector)
			Deflector	= Layer[0];
			std::sort	(Layer.begin()+1,Layer.end(),cmp_defl);

			// Select first deflectors which can fit
			int maxarea = lmap_size*lmap_size*6;	// Max up to 6 lm selected
			int curarea = 0;
			for (it=1; it<(int)Layer.size(); it++)
			{
				int		defl_area	= Layer[it]->GetLayer(L_base)->Area();
				if (curarea + defl_area > maxarea) break;
				curarea		+=	defl_area;
				SEL.push_back(Layer[it]);
			}
			if (SEL.empty()) 
			{
				// No deflectors found to merge
				// Simply transfer base deflector to _new list
				deflNew.push_back(Deflector);
				g_deflectors.erase(g_deflectors.begin());
			} else {
				// Transfer rects
				SEL.push_back(Deflector);
				for (int K=0; K<(int)SEL.size(); K++)
				{
					_rect	T; 
					T.a.set	(0,0);
					T.b.set	(SEL[K]->lm.dwWidth+2*BORDER-1, SEL[K]->lm.dwHeight+2*BORDER-1);
					T.iArea = SEL[K]->iArea;
					selected.push_back	(T);
					perturb.push_back	(K);
				}
				
				// Sort by size decreasing and startup
				std::sort			(perturb.begin(),perturb.end(),cmp_rect);
				InitSurface			();
				int id				= perturb[0];
				_rect &First		= selected[id];
				_rect_register		(First,SEL[id],FALSE);
				best.push_back		(First);
				best_seq.push_back	(id);
				brect.set			(First);
				
				// Process 
				collected.reserve	(SEL.size());
				for (int R=1; R<(int)selected.size(); R++) 
				{
					int ID = perturb[R];
					if (_rect_place(selected[ID],SEL[ID])) 
					{
						brect.Merge			(collected.back());
						best.push_back		(collected.back());
						best_seq.push_back	(ID);
					}
					Progress(float(R)/float(selected.size()));
				}
				R_ASSERT	(brect.a.x==0 && brect.a.y==0);
				
				//  Analyze resuls
				clMsg("%3d / %3d - [%d,%d]",best.size(),selected.size(),brect.SizeX(),brect.SizeY());
				CDeflector*	pDEFL = xr_new<CDeflector>();
				pDEFL->lm.bHasAlpha = FALSE;
				pDEFL->lm.dwWidth   = lmap_size;
				pDEFL->lm.dwHeight  = lmap_size;
				for (K = 0; K<(int)best.size(); K++) 
				{
					int			iRealIndex	= best_seq	[K];
					_rect&		Place		= best		[K];
					_point&		Offset		= Place.a;
					BOOL		bRotated;
					b_texture&	T			= SEL[iRealIndex]->lm;
					int			T_W			= (int)T.dwWidth	+ 2*BORDER;
					int			T_H			= (int)T.dwHeight	+ 2*BORDER;
					if (Place.SizeX() == T_W) {
						R_ASSERT(Place.SizeY() == T_H);
						bRotated = FALSE;
					} else {
						R_ASSERT(Place.SizeX() == T_H);
						R_ASSERT(Place.SizeY() == T_W);
						bRotated = TRUE;
					}
					
					// Merge
					pDEFL->Capture		(SEL[iRealIndex],Offset.x,Offset.y,Place.SizeX(),Place.SizeY(),bRotated);
					
					// Destroy old deflector
					vecDeflIt		OLD = std::find(g_deflectors.begin(),g_deflectors.end(),SEL[iRealIndex]);
					VERIFY			(OLD!=g_deflectors.end());
					g_deflectors.erase(OLD);
					xr_delete		(SEL[iRealIndex]);
				}
				pDEFL->Save			();
				deflNew.push_back	(pDEFL);
				
				// Cleanup
				SEL.clear			();
				collected.clear		();
				selected.clear		();
				perturb.clear		();
				best.clear			();
				best_seq.clear		();
				brect.iArea			= INT_MAX;
			}
			Progress(1.f-float(g_deflectors.size())/float(dwOldCount));
		}
	}
	
	R_ASSERT(g_deflectors.empty());
	g_deflectors = deflNew;
	clMsg	("%d lightmaps builded",g_deflectors.size());
}

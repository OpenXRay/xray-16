#include "stdafx.h"
#include "compiler.h"

//-------------------------------------------------------------------------------------------------------
#include "resource.h"

extern HWND logWindow;

int	dimX,dimZ;
struct	Texel {
	vertex*	N;
	u32	depth;
};


Texel*	texels=0;

float minH=flt_max, maxH=flt_min;
void InternalRender()
{
	Fvector S,P;
	LevelBB.getsize(S);
	dimX	= iCeil(S.x/g_params.fPatchSize);
	dimZ	= iCeil(S.z/g_params.fPatchSize);

	// allocation
	int		msize = dimX*dimZ*sizeof(Texel);
	texels	= (Texel*)xr_malloc(msize);
	ZeroMemory(texels,msize);

	// rasterization
	for (u32 i=0; i<g_nodes.size(); i++)
	{
		vertex&	N	= g_nodes[i];
		P.sub	(N.Pos, LevelBB.min);
		int		nx	= iFloor(P.x/g_params.fPatchSize+0.5f);	clamp(nx,0,dimX-1);
		int		nz	= iFloor(P.z/g_params.fPatchSize+0.5f);	clamp(nz,0,dimZ-1);

		Texel&	T	= texels[(dimZ-nz-1)*dimX+nx];
		T.depth++;
		if (T.N)	{ if (N.Pos.y>T.N->Pos.y)	T.N = &N; }
		else		T.N = &N;
	}

	// limits
	for (int t=0; t<dimX*dimZ; t++)
	{
		Texel& T	= texels[t];
		if (T.N)	{
			minH		= _min(minH,T.N->Pos.y);
			maxH		= _max(maxH,T.N->Pos.y);
		}
	}
}
void pixel(HDC dc, int x, int y, u32 C)
{
	int				_x=x*3,_y=y*3;
	for (int i=0; i<3; i++)
	{
		for (int j=0; j<3; j++)
		{
			SetPixel (dc,_x+j,_y+i,C);
		}
	}
}

void ShowDepth(HWND hw)
{
	HDC	dc = GetDC	(hw);

	for (int z=0; z<dimZ; z++)
	{
		for (int x=0; x<dimX; x++)
		{
			Texel&	T	= texels[z*dimX+x];
			int		d	= T.depth;
			u32	C	= RGB(d*32,d*32,d*32);
			if (0==d)	C = RGB(0,127,0);
			pixel	(dc,x,z,C);
		}
	}

	ReleaseDC		(hw, dc);
}
void ShowHeight(HWND hw)
{
	HDC	dc = GetDC	(hw);

	for (int z=0; z<dimZ; z++)
	{
		for (int x=0; x<dimX; x++)
		{
			Texel&	T	= texels[z*dimX+x];
			u32	C	= RGB(0,127,0);
			if (T.N)	{
				float	h	= ((T.N->Pos.y)-minH)/(maxH-minH);
				u32	c	= iFloor(h*255.f);
				C			= RGB(c,c,c);
			}
			pixel	(dc,x,z,C);
		}
	}

	ReleaseDC		(hw, dc);
}
void ShowLight(HWND hw)
{
	HDC	dc = GetDC	(hw);

	for (int z=0; z<dimZ; z++)
	{
		for (int x=0; x<dimX; x++)
		{
			Texel&	T	= texels[z*dimX+x];
			u32	C	= RGB(0,127,0);
			if (T.N)	{
				int		l	= iFloor(T.N->LightLevel*255.f);
				clamp		(l,0,255);
				u32	c	= u32(l);
				C			= RGB(c,c,c);
			}
			pixel	(dc,x,z,C);
		}
	}

	ReleaseDC		(hw, dc);
}
void ShowNormals(HWND hw)
{
	HDC	dc = GetDC	(hw);

	for (int z=0; z<dimZ; z++)
	{
		for (int x=0; x<dimX; x++)
		{
			Texel&	T	= texels[z*dimX+x];
			u32	C	= RGB(0,127,0);
			if (T.N)	{
				Fvector	N	= T.N->Plane.n;
				C			= RGB(iFloor(_abs(N.x)*255),iFloor(_abs(N.y)*255),iFloor(_abs(N.z)*255));
			}
			pixel	(dc,x,z,C);
		}
	}

	ReleaseDC		(hw, dc);
}
void ShowSectors(HWND hw)
{
	HDC	dc = GetDC	(hw);

	for (int z=0; z<dimZ; z++)
	{
		for (int x=0; x<dimX; x++)
		{
			Texel&	T	= texels[z*dimX+x];
			u32	C	= RGB(0,127,0);
			if (T.N)	{
				int		s	= T.N->Sector;
				if (s==InvalidSector)	C = RGB(255,0,0);
				else					C = RGB(s*32,s*32,s*32);
			}
			pixel	(dc,x,z,C);
		}
	}

	ReleaseDC		(hw, dc);
}
void ShowCover(HWND hw, int direction)
{
	HDC	dc = GetDC	(hw);

	for (int z=0; z<dimZ; z++)
	{
		for (int x=0; x<dimX; x++)
		{
			Texel&	T	= texels[z*dimX+x];
			u32	C	= RGB(0,127,0);
			if (T.N)	{
				int		v	=	iFloor(T.N->high_cover[direction]*255.f);
				clamp	(v,0,255);
				C			=	RGB(v,v,v);
			}
			pixel	(dc,x,z,C);
		}
	}

	ReleaseDC		(hw, dc);
}
IC bool isBorder(vertex& N, int dir)
{
	if	(N.n[dir]==InvalidNode)	return true;

	vertex&	C = g_nodes[N.n[dir]];
	if (N.Group!=C.Group)		return true;

	return false;
}
void ShowSubdiv(HWND hw)
{
	HDC	dc = GetDC	(hw);

	u32 CB = RGB(255,0,0);
	for (int z=0; z<dimZ; z++)
	{
		for (int x=0; x<dimX; x++)
		{
			Texel&	T	= texels[z*dimX+x];
			if (T.N)	{
				pixel	(dc,x,z,RGB(127,127,127));
				vertex&	N = *T.N;

				int		_x=x*3,_y=z*3;
				if		(isBorder(N,0))	{		// left
					SetPixel(dc,_x,_y+0,CB);
					SetPixel(dc,_x,_y+1,CB);
					SetPixel(dc,_x,_y+2,CB);
				}
				if		(isBorder(N,1))	{		// fwd
					SetPixel(dc,_x+0,_y,CB);
					SetPixel(dc,_x+1,_y,CB);
					SetPixel(dc,_x+2,_y,CB);
				}
				if		(isBorder(N,2))	{		// right
					SetPixel(dc,_x+2,_y+0,CB);
					SetPixel(dc,_x+2,_y+1,CB);
					SetPixel(dc,_x+2,_y+2,CB);
				}
				if		(isBorder(N,3))	{		// back
					SetPixel(dc,_x+0,_y+2,CB);
					SetPixel(dc,_x+1,_y+2,CB);
					SetPixel(dc,_x+2,_y+2,CB);
				}
			} else {
				pixel	(dc,x,z,RGB(0,127,0));
			}
		}
	}

	ReleaseDC		(hw, dc);
}

static BOOL CALLBACK disp_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch( message )
	{
	case WM_INITDIALOG:
		break;
	case WM_DESTROY:
	case WM_CLOSE:
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:				EndDialog(hWnd, IDOK);	break;
		case ID_VIEW_NDEPTH:	ShowDepth(hWnd);		break;
		case ID_VIEW_NHEIGHT:	ShowHeight(hWnd);		break;
		case ID_VIEW_NSECTOR:	ShowSectors(hWnd);		break;
		case ID_VIEW_NNORMALS:	ShowNormals(hWnd);		break;
		case ID_VIEW_NLIGHT:	ShowLight(hWnd);		break;
		case ID_VIEW_NSUBDIV:	ShowSubdiv(hWnd);		break;
		case ID_VIEW_COVER1:	ShowCover(hWnd,0);		break;
		case ID_VIEW_COVER2:	ShowCover(hWnd,1);		break;
		case ID_VIEW_COVER3:	ShowCover(hWnd,2);		break;
		case ID_VIEW_COVER4:	ShowCover(hWnd,3);		break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void xrDisplay		()
{
	InternalRender	();
	DialogBox		(HINSTANCE(GetModuleHandle(0)),MAKEINTRESOURCE(IDD_NVIEW),logWindow,disp_proc);
	xr_free			(texels);
}

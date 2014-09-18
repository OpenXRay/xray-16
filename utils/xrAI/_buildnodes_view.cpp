#include "stdafx.h"
#include "resource.h"

static HWND hw;
extern HWND logWindow;

void InternalRender()
{

}

LRESULT CALLBACK disp_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch( msg )
	{
	case WM_INITDIALOG:
		hw	= hWnd;
		InternalRender	();
		break;
	case WM_DESTROY:
	case WM_CLOSE:
		break;
	case WM_COMMAND:
		switch (LOWORD(wp))
		{
		case ID_OK:				EndDialog(hw, ID_OK);	break;
		case ID_VIEW_NDEPTH:	ShowDepth(hw);			break;
		case ID_VIEW_NHEIGHT:	ShowHeight(hw);			break;
		case ID_VIEW_NSECTOR:	ShowSectors(hw);		break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void disp_point(int x, int y)
{
	HDC				hdc;
	
	hdc = GetDC		(hw);
	
	u32			C = RGB(0,0,0);
	int				_x=x*2,_y=y*2;
	SetPixel		(hdc,_x,	_y,		C);
	SetPixel		(hdc,_x+1,	_y,		C);
	SetPixel		(hdc,_x,	_y+1,	C);
	SetPixel		(hdc,_x+1,	_y+1,	C);

	ReleaseDC		(hw, hdc);
}

void disp_run()
{
	DialogBox(HINSTANCE(GetModuleHandle(0)),MAKEINTRESOURCE(IDD_NVIEW),logWindow,disp_proc);
}

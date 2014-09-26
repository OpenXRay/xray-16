// SideBarCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "peerlobby.h"
#include "SideBarCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSideBarCtrl

CSideBarCtrl::CSideBarCtrl()
{
}

CSideBarCtrl::~CSideBarCtrl()
{
}


BEGIN_MESSAGE_MAP(CSideBarCtrl, CStatic)
	//{{AFX_MSG_MAP(CSideBarCtrl)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSideBarCtrl message handlers

BOOL CSideBarCtrl::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
	
//	return CStatic::OnEraseBkgnd(pDC);
}

void CSideBarCtrl::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	CRect rcClient;
	GetClientRect(rcClient);

	DrawGradient(&dc, rcClient, RGB(0,0,0), RGB(0,255,0), TRUE );

	int oldBkMode = dc.SetBkMode(TRANSPARENT);
	HFONT oldFont = (HFONT)dc.SelectObject( (HFONT)::GetStockObject(ANSI_VAR_FONT) );
	COLORREF oldTextColor = dc.SetTextColor( RGB(0,204,0) );

	//
	// Bottom Text
	//
	CString str;
	GetWindowText(str);

	CRect rcText = rcClient;
	rcText.DeflateRect(3,3);

	dc.DrawText( str, rcText, DT_BOTTOM|DT_CENTER|DT_SINGLELINE );

	rcText.OffsetRect(-1,-1);
	
	dc.SetTextColor( RGB(0,0,0) );
	
	dc.DrawText( str, rcText, DT_BOTTOM|DT_CENTER|DT_SINGLELINE );


	//
	// Top Text
	//
	CFont fntBold;
	LOGFONT lf = {0};
	::GetObject((HFONT)::GetStockObject(ANSI_VAR_FONT),sizeof(LOGFONT),&lf);
	lf.lfWeight = FW_SEMIBOLD;
	fntBold.CreateFontIndirect(&lf);
	
	dc.SelectObject(fntBold);

	dc.SetTextColor(RGB(255,255,255));

	dc.DrawText("\nGameSpy\n\n\"PeerLobby\"\n\nSample\nApplication",-1,rcClient,DT_CENTER);


	//
	// Cleanup
	//

	dc.SetTextColor(oldTextColor);
	dc.SetBkMode(oldBkMode);
	dc.SelectObject(oldFont);

	fntBold.DeleteObject();
}

//
// Helpers
//
BYTE findMidTone( BYTE bColorOne, BYTE bColorTwo ) 
{
	BYTE bResult = 0;
	WORD wMax = (WORD)(bColorOne + bColorTwo);
	if( wMax > 0 )
	{
		bResult = ( (BYTE)(wMax/2) );
	}
	return bResult;
}

COLORREF findMidColor( COLORREF cOne, COLORREF cTwo ) 
{
	return( RGB( findMidTone( GetRValue(cOne), GetRValue(cTwo) ), 
							 findMidTone( GetGValue(cOne), GetGValue(cTwo) ), 
							 findMidTone( GetBValue(cOne), GetBValue(cTwo) ) ) );
}

/*****************************************************************************
* NAME: 
*  DrawGradient
* 
* DESCRIPTION: 
*  Found this as part of an overly complicated gradient progress bar on codeguru
*  So, I simplified it for my own uses :)   -Lumberjack
* 
*******************************************************************************/
void CSideBarCtrl::DrawGradient(CDC* pDC, const CRect &rcGrad, COLORREF clrStart, COLORREF clrEnd, BOOL bVertical, COLORREF clrDither /*=0xFFFFFFFF*/ )
{
	// Split colors to RGB chanels, find chanel with maximum difference 
	// between the start and end colors. This distance will determine 
	// number of steps of gradient
	int r = (GetRValue(clrEnd) - GetRValue(clrStart));
	int g = (GetGValue(clrEnd) - GetGValue(clrStart));
	int b = (GetBValue(clrEnd) - GetBValue(clrStart));
	int nSteps = max(abs(r), max(abs(g), abs(b)));
	// if number of pixels in gradient less than number of steps - 
	// use it as numberof steps
	int nPixels = rcGrad.Width();
	nSteps = min(nPixels, nSteps);
	if(nSteps == 0) nSteps = 1;

	float rStep = (float)r/nSteps;
	float gStep = (float)g/nSteps;
	float bStep = (float)b/nSteps;

	r = GetRValue(clrStart);
	g = GetGValue(clrStart);
	b = GetBValue(clrStart);

	BOOL fLowColor = pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE;
	if(!fLowColor && nSteps > 1)
		if(pDC->GetDeviceCaps(BITSPIXEL)*pDC->GetDeviceCaps(PLANES) < 8)
			nSteps = 1; // for 16 colors no gradient

	float nSpacePerStep = bVertical?(float)rcGrad.Height()/nSteps:(float)rcGrad.Width()/nSteps;
	CRect rcFill(rcGrad);
	CBrush br;

	// Start filling
	for (int i = 0; i < nSteps; i++) 
	{
		if( bVertical )
		{
			rcFill.top = rcGrad.top + (int)(nSpacePerStep * i);
			rcFill.bottom = rcGrad.top + (int)(nSpacePerStep * (i+1));
			if(i == nSteps-1)	//last step (because of problems with float)
				rcFill.bottom = rcGrad.bottom;
		}
		else
		{
			rcFill.left = rcGrad.left + (int)(nSpacePerStep * i);
			rcFill.right = rcGrad.left + (int)(nSpacePerStep * (i+1));
			if(i == nSteps-1)	//last step (because of problems with float)
				rcFill.right = rcGrad.right;
		}

		COLORREF clrFill = RGB(r + (int)(i * rStep), g + (int)(i * gStep), b + (int)(i * bStep));

		if( fLowColor )
		{
			br.CreateSolidBrush(clrFill);
			// CDC::FillSolidRect is faster, but it does not handle 8-bit color depth
			pDC->FillRect(&rcFill, &br);
			br.DeleteObject();
		}
		else
		{
			if( 0xFFFFFFFF != clrDither )
			{
				COLORREF crExisting = 0;
				for( int nHoriz = rcFill.left; nHoriz < rcFill.right; nHoriz++ )
				{
					for( int nVert = rcFill.top; nVert < rcFill.bottom; nVert++ )
					{
						crExisting = pDC->GetPixel(nHoriz,nVert);
						if( crExisting == clrDither )
						{
							pDC->SetPixel(nHoriz,nVert,findMidColor(clrFill,clrDither) );
						}
						else
						{
							pDC->SetPixel(nHoriz,nVert,clrFill);
						}
					}
				}
			}
			else
			{
				pDC->FillSolidRect(&rcFill, clrFill);
			}
		}
	}
}
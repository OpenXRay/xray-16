#include "stdafx.h"
#pragma hdrstop

#include "tlsprite.h"
#include "texture.h"
#include "ui_main.h"
#include "d3dutils.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CTLSprite::CTLSprite	( )
{
	mesh.setcolor	(0xFFFFFFFF);
	mesh.setdepth	( );
	mesh.settex		(0,0,1,1);
}

CTLSprite::~CTLSprite( )
{
}

void CTLSprite::Render(Fvector &pos, float radius, bool bFixedSize, u32 clr)
{
	FVF::TL			P;
    float 			cx,cy,sz;

	P.transform		( pos, EDevice.mFullTransform );
    cx				= EDevice._x2real(P.p.x);
    cy				= EDevice._y2real(P.p.y);
    if (bFixedSize) sz = EDevice._x2real(radius)-EDevice._x2real(0);
    else			sz = UI->GetRenderWidth()*radius/P.p.w;
    mesh.m[0].p.x = cx-sz;	mesh.m[0].p.y = cy+sz;
    mesh.m[1].p.x = cx-sz;	mesh.m[1].p.y = cy-sz;
    mesh.m[3].p.x = cx+sz;	mesh.m[3].p.y = cy-sz;
    mesh.m[2].p.x = cx+sz;	mesh.m[2].p.y = cy+sz;
    mesh.setdepth	( P.p.z, P.p.w );
    mesh.setcolor	( clr );

    DU_impl.DrawPrimitiveTL(D3DPT_TRIANGLESTRIP,2,(FVF::TL*)&mesh,4,false,false);
}

void CTLSprite::Render(Fvector &pos, u32 color, float radius, float angle)
{
	FVF::TL			P;

	P.transform     (pos, EDevice.mFullTransform);
    if (P.p.w<=0) 	return;

	float cx		= EDevice._x2real(P.p.x);
	float cy		= EDevice._y2real(P.p.y);

	float sz		= (UI->GetRenderWidth()*radius)/P.p.w;
	if (sz<1.5f) 	return;

	// Rotation
	float			_sin1=_sin(angle),_cos1=_cos(angle);
    float			_sin2=_sin(angle+PI_DIV_2),_cos2=_cos(angle+PI_DIV_2);

	mesh.m[0].p.x = cx+sz*_sin1;	mesh.m[0].p.y = cy+sz*_cos1;
	mesh.m[1].p.x = cx-sz*_sin2;	mesh.m[1].p.y = cy-sz*_cos2;
	mesh.m[2].p.x = cx+sz*_sin2;	mesh.m[2].p.y = cy+sz*_cos2;
	mesh.m[3].p.x = cx-sz*_sin1;	mesh.m[3].p.y = cy-sz*_cos1;

	mesh.setdepth	(P.p.z, P.p.w);
	mesh.setcolor	(color);

    DU_impl.DrawPrimitiveTL(D3DPT_TRIANGLESTRIP,2,(FVF::TL*)&mesh,4,false,false);
}

void CTLSprite::Render( Fvector &pos, u32 color, float radius, const Fvector& D ){
	Fvector			P1,P2;
    P1.mad			(pos,D,-radius);
    P2.mad 			(pos,D,radius);

	FVF::TL			s1,s2;
	s1.transform	(P1,EDevice.mFullTransform);
	s2.transform	(P2,EDevice.mFullTransform);

    if ((s1.p.w<=0)||(s2.p.w<=0)) return;

	float l1 		= radius/s1.p.w*1.4142f;
    float l2 		= radius/s2.p.w*1.4142f;

	Fvector2		dir,R;
	dir.set			(s2.p.x-s1.p.x,s2.p.y-s1.p.y);
	dir.norm		();
	R.cross			(dir);

	mesh.m[0].p.x 	= EDevice._x2real(s1.p.x+l1*R.x);	mesh.m[0].p.y = EDevice._y2real(s1.p.y+l1*R.y); mesh.m[0].p.z = s2.p.z; mesh.m[0].p.w = s2.p.w;
	mesh.m[1].p.x 	= EDevice._x2real(s2.p.x+l2*R.x);	mesh.m[1].p.y = EDevice._y2real(s2.p.y+l2*R.y); mesh.m[1].p.z = s2.p.z; mesh.m[1].p.w = s2.p.w;
	mesh.m[2].p.x 	= EDevice._x2real(s1.p.x-l1*R.x);	mesh.m[2].p.y = EDevice._y2real(s1.p.y-l1*R.y); mesh.m[2].p.z = s2.p.z; mesh.m[2].p.w = s2.p.w;
	mesh.m[3].p.x 	= EDevice._x2real(s2.p.x-l2*R.x);	mesh.m[3].p.y = EDevice._y2real(s2.p.y-l2*R.y); mesh.m[3].p.z = s2.p.z; mesh.m[3].p.w = s2.p.w;

	mesh.setcolor	(color);

    DU_impl.DrawPrimitiveTL(D3DPT_TRIANGLESTRIP,2,(FVF::TL*)&mesh,4,false,false);
/*
	float radius	= size*0.5f;

	Fvector			P1,P2;
    P1.direct		(pos,D,radius);
    P2.direct		(pos,D,-radius);

	FVF::TL			s1,s2;
	s1.transform	(P1,EDevice.m_FullTransform);
	s2.transform	(P2,EDevice.m_FullTransform);

    if ((s1.p.w<=0)||(s2.p.w<=0)) return;

	float l1 = radius/s1.p.w*1.4142f;
    float l2 = radius/s2.p.w*1.4142f;

	Fvector2		dir,R;
	dir.set			(s2.p.x-s1.p.x,s2.p.y-s1.p.y);
	dir.norm		();
	R.cross			(dir);

	mesh.m[0].p.x = X_TO_REAL(s1.p.x+l1*R.x);	mesh.m[0].p.y = Y_TO_REAL(s1.p.y+l1*R.y); mesh.m[0].p.z = s1.p.z; mesh.m[0].p.w = s1.p.w;
	mesh.m[1].p.x = X_TO_REAL(s2.p.x+l2*R.x);	mesh.m[1].p.y = Y_TO_REAL(s2.p.y+l2*R.y); mesh.m[1].p.z = s2.p.z; mesh.m[1].p.w = s2.p.w;
	mesh.m[2].p.x = X_TO_REAL(s1.p.x-l1*R.x);	mesh.m[2].p.y = Y_TO_REAL(s1.p.y-l1*R.y); mesh.m[2].p.z = s1.p.z; mesh.m[2].p.w = s1.p.w;
	mesh.m[3].p.x = X_TO_REAL(s2.p.x-l2*R.x);	mesh.m[3].p.y = Y_TO_REAL(s2.p.y-l2*R.y); mesh.m[3].p.z = s2.p.z; mesh.m[3].p.w = s2.p.w;
//	mesh.m[1].p.x = X_TO_REAL(s2.p.x);			mesh.m[1].p.y = Y_TO_REAL(s2.p.y); 		  mesh.m[1].p.z = s2.p.z; mesh.m[1].p.w = s2.p.w;
//    mesh.m[1].tu  = 0.5f; mesh.m[1].tv = 0.f;

	mesh.setcolor	(RGBA_MAKE(iFloor(color.r),iFloor(color.g),iFloor(color.b),iFloor(color.a)));

    EDevice.DP	(D3DPT_TRIANGLESTRIP, FVF::F_TL, &mesh, 4);
*/
/*    Fvector2 		dir;
    dir.x			= p1.x-p0.x;
    dir.y			= p1.y-p0.y;
    dir.norm_safe	();

    Fvector P0,P1;
    P0.direct		(pos,);

	FVF::TL			P;
	P.transform		(pos,EDevice.m_FullTransform);
	float sz		= UI->GetRenderWidth()*radius/P.p.w;
	float sz0		= radius/P.p.w;//(radius/p0.w)/2;
	float sz1		= radius/P.p.w;//radius/p1.w;
	Fvector2		D,R;
    D.x				= (dir.x-dir.y)*0.7071f;
    D.y				= (dir.x+dir.y)*0.7071f;
	R.cross			(D);

	float cx		= X_TO_REAL(P.p.x);
	float cy		= Y_TO_REAL(P.p.y);

	mesh.m[0].p.x = X_TO_REAL(p0.x+sz0*R.x);	mesh.m[0].p.y = Y_TO_REAL(p0.y+sz0*R.y);
	mesh.m[1].p.x = X_TO_REAL(p1.x+sz1*R.x);	mesh.m[1].p.y = Y_TO_REAL(p1.y+sz1*R.y);
	mesh.m[2].p.x = X_TO_REAL(p0.x-sz0*R.x);	mesh.m[2].p.y = Y_TO_REAL(p0.y-sz0*R.y);
	mesh.m[3].p.x = X_TO_REAL(p1.x-sz1*R.x);	mesh.m[3].p.y = Y_TO_REAL(p1.y-sz1*R.y);

	mesh.setdepth	(P.p.z, P.p.w);
	mesh.setcolor	(RGBA_MAKE(iFloor(color.r),iFloor(color.g),iFloor(color.b),iFloor(color.a)));

    EDevice.DP	(D3DPT_TRIANGLESTRIP, FVF::F_TL, &mesh, 4);
*/
}

/*
void CTLSprite::UpdateTC(){
    float		tu,tv;
    int			frame;

	float fSPF 	= fPlayTime/float(iCount);
	fLocalTime	+=EDevice.fTimeDelta;
	if (fLocalTime>=fPlayTime) fLocalTime-=fPlayTime;
	frame		= iFloor( fLocalTime/fSPF );

	tu			=(frame%iFrameDim)*fTexSize;
	tv			=(frame/iFrameDim)*fTexSize;

    mesh.m[0].tu=tu+fTexSize;	mesh.m[0].tv=tv+fTexSize;
    mesh.m[1].tu=tu;			mesh.m[1].tv=tv+fTexSize;
    mesh.m[2].tu=tu;			mesh.m[2].tv=tv;
    mesh.m[3].tu=tu+fTexSize;	mesh.m[3].tv=tv;
}

void CTLSprite::Render( Fvector &pos, float alpha, float size, float scale )
{
	FTLvertex		P;
	FPcolor			cur;
	float			s_da, s_da2, c_da, c_da2;
	uchar			cc;

	P.s_transform	( pos, UI->m_FullTransform );
	float cx		= X_TO_REAL(P.sx);
	float cy		= Y_TO_REAL(P.sy);

	float dx		= size/P.rhw * scale * UI->GetRenderWidth() / 2;
	float dy		= size/P.rhw / scale * UI->GetRenderWidth() / 2;

	cc				= uchar(255.f*alpha);
	cur.set			(cc,cc,cc,255);

	c_da	        = dy*0.707106f;
	s_da            = -dx*0.707106f;

	mesh.m[0].sx = cx + s_da;	mesh.m[0].sy = cy + c_da;
	mesh.m[1].sx = cx + s_da;	mesh.m[1].sy = cy - c_da;
	mesh.m[3].sx = cx - s_da;	mesh.m[3].sy = cy - c_da;
	mesh.m[2].sx = cx - s_da;	mesh.m[2].sy = cy + c_da;

	mesh.setdepth	( P.sz, P.rhw );
	mesh.setcolor	( cur );

    CDX( UI->d3d()->DrawPrimitive( D3DPT_TRIANGLESTRIP, FTLVERTEX, &mesh, 4, 0 ));
    UI->stat->lRenderPolyCount += 2;
}*/

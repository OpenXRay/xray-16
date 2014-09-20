//----------------------------------------------------
// file: D3DUtils.h
//----------------------------------------------------

#ifndef D3DUtilsH
#define D3DUtilsH
#include "..\..\Include\xrRender\DrawUtils.h"
//----------------------------------------------------

#ifdef _EDITOR
#	define DU_DRAW_DIP	EDevice.DIP
#	define DU_DRAW_DP	EDevice.DP
#else
#	define DU_DRAW_DIP	RCache.dbg_DIP
#	define DU_DRAW_DP	RCache.dbg_DP
#endif

struct SPrimitiveBuffer{
    ref_geom				pGeom;
    u32						v_cnt;
    u32						i_cnt;
    D3DPRIMITIVETYPE 		p_type;
    u32						p_cnt;
	typedef fastdelegate::FastDelegate0<> TOnRender;
    TOnRender				OnRender;
    void xr_stdcall			RenderDIP()	{DU_DRAW_DIP(p_type,pGeom,0,0,v_cnt,0,p_cnt);}
    void xr_stdcall			RenderDP()	{DU_DRAW_DP	(p_type,pGeom,0,p_cnt);}
public:
                            SPrimitiveBuffer():OnRender(0),pGeom(0){;}
    void					CreateFromData(D3DPRIMITIVETYPE _pt, u32 _p_cnt, u32 FVF, LPVOID vertices, u32 _v_cnt, u16* indices=0, u32 _i_cnt=0);
    void					Destroy();
    void					Render(){OnRender();}
};

//----------------------------------------------------
// Utilities
//----------------------------------------------------
class ECORE_API CDrawUtilities: public CDUInterface, public pureRender{
	SPrimitiveBuffer		m_SolidCone;
	SPrimitiveBuffer		m_WireCone;
    SPrimitiveBuffer		m_SolidSphere;
    SPrimitiveBuffer		m_WireSphere;
    SPrimitiveBuffer		m_SolidSpherePart;
    SPrimitiveBuffer		m_WireSpherePart;
    SPrimitiveBuffer		m_SolidCylinder;
    SPrimitiveBuffer		m_WireCylinder;
    SPrimitiveBuffer		m_SolidBox;
    SPrimitiveBuffer		m_WireBox;
	CGameFont* 				m_Font;
public:
	ref_geom 	vs_L;
	ref_geom 	vs_TL;
	ref_geom 	vs_LIT;
protected:
    FVF::L*		m_DD_pv;
    FVF::L*		m_DD_pv_start;
    u32 		m_DD_base;
    BOOL		m_DD_wire;
	void 		DD_DrawFace_flush		(BOOL try_again);
public:
	void 		DD_DrawFace_begin		(BOOL bWire);
	void 		DD_DrawFace_push		(const Fvector& p0, const Fvector& p1, const Fvector& p2, u32 clr);
	void 		DD_DrawFace_end			();
public:
	CDrawUtilities()
    {
        vs_L	= 0;
        vs_TL	= 0;
        vs_LIT	= 0;
        m_DD_pv			= 0;
        m_DD_pv_start	= 0;
        m_DD_base		= 0;
		m_Font	= 0;
    }
    
    void OnDeviceCreate	();
    virtual void __stdcall OnDeviceDestroy();

	void UpdateGrid(int number_of_cell, float square_size, int subdiv=10);

    //----------------------------------------------------
    virtual void __stdcall DrawCross(const Fvector& p, float szx1, float szy1, float szz1, float szx2, float szy2, float szz2, u32 clr, BOOL bRot45=false);
    virtual void __stdcall DrawCross(const Fvector& p, float sz, u32 clr, BOOL bRot45=false){ DrawCross(p, sz,sz,sz, sz,sz,sz, clr, bRot45); }
    virtual void __stdcall DrawEntity(u32 clr, ref_shader s);
    virtual void __stdcall DrawFlag(const Fvector& p, float heading, float height, float sz, float sz_fl, u32 clr, BOOL bDrawEntity);
    virtual void __stdcall DrawRomboid(const Fvector& p, float radius, u32 clr);
	virtual void __stdcall DrawJoint(const Fvector& p, float radius, u32 clr);

    virtual void __stdcall DrawSpotLight(const Fvector& p, const Fvector& d, float range, float phi, u32 clr);
    virtual void __stdcall DrawDirectionalLight(const Fvector& p, const Fvector& d, float radius, float range, u32 clr);
    virtual void __stdcall DrawPointLight(const Fvector& p, float radius, u32 clr);

    virtual void __stdcall DrawSound(const Fvector& p, float radius, u32 clr);
    virtual void __stdcall DrawLineSphere(const Fvector& p, float radius, u32 clr, BOOL bCross);

	virtual void __stdcall dbgDrawPlacement(const Fvector& p, int sz, u32 clr, LPCSTR caption=0, u32 clr_font=0xffffffff);
    virtual void __stdcall dbgDrawVert(const Fvector& p0, u32 clr, LPCSTR caption=0);
    virtual void __stdcall dbgDrawEdge(const Fvector& p0,	const Fvector& p1, u32 clr, LPCSTR caption=0);
    virtual void __stdcall dbgDrawFace(const Fvector& p0,	const Fvector& p1, const Fvector& p2, u32 clr, LPCSTR caption=0);

    virtual void __stdcall DrawFace(const Fvector& p0,	const Fvector& p1, const Fvector& p2, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire);
    virtual void __stdcall DrawLine(const Fvector& p0,	const Fvector& p1, u32 clr);
    virtual void __stdcall DrawLink(const Fvector& p0, const Fvector& p1, float sz, u32 clr);
    IC virtual void __stdcall DrawFaceNormal(const Fvector& p0, const Fvector& p1, const Fvector& p2, float size, u32 clr){
        Fvector N,C,P; N.mknormal(p0,p1,p2); C.set(p0);C.add(p1);C.add(p2);C.div(3);
        P.mad(C,N,size);
        DrawLine(C,P,clr);}
    IC virtual void __stdcall DrawFaceNormal(const Fvector* p, float size, u32 clr){DrawFaceNormal(p[0],p[1],p[2],size,clr);}
    IC virtual void __stdcall DrawFaceNormal(const Fvector& C, const Fvector& N, float size, u32 clr){
        Fvector P; P.mad(C,N,size);DrawLine(C,P,clr);}
    virtual void __stdcall DrawSelectionBox(const Fvector& center, const Fvector& size, u32* c=0);
    IC virtual void __stdcall DrawSelectionBoxB(const Fbox& box, u32* c=0){
        Fvector S,C;
        box.getsize(S);
        box.getcenter(C);
        DrawSelectionBox(C,S,c);
    }
    virtual void __stdcall DrawIdentSphere	(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w);
    virtual void __stdcall DrawIdentSpherePart(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w);
    virtual void __stdcall DrawIdentCone		(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w);
	virtual void __stdcall DrawIdentCylinder	(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w);
    virtual void __stdcall DrawIdentBox		(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w);

    virtual void __stdcall DrawBox	(const Fvector& offs, const Fvector& Size, BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w);
    virtual void __stdcall DrawAABB	(const Fvector& p0, const Fvector& p1, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire);
    virtual void __stdcall DrawAABB	(const Fmatrix& parent, const Fvector& center, const Fvector& size, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire);
    virtual void __stdcall DrawOBB	(const Fmatrix& parent, const Fobb& box, u32 clr_s, u32 clr_w);
	virtual void __stdcall DrawSphere	(const Fmatrix& parent, const Fvector& center, float radius, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire);
	virtual void __stdcall DrawSphere	(const Fmatrix& parent, const Fsphere& S, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire){DrawSphere(parent,S.P,S.R,clr_s,clr_w,bSolid,bWire);}
	virtual void __stdcall DrawCylinder(const Fmatrix& parent, const Fvector& center, const Fvector& dir, float height, float radius, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire);
	virtual void __stdcall DrawCone	(const Fmatrix& parent, const Fvector& apex, const Fvector& dir, float height, float radius, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire);
    virtual void __stdcall DrawPlane	(const Fvector& center, const Fvector2& scale, const Fvector& rotate, u32 clr_s, u32 clr_w, BOOL bCull, BOOL bSolid, BOOL bWire);
    virtual void __stdcall DrawPlane	(const Fvector& p, const Fvector& n, const Fvector2& scale, u32 clr_s, u32 clr_w, BOOL bCull, BOOL bSolid, BOOL bWire);
    virtual void __stdcall DrawRectangle(const Fvector& o, const Fvector& u, const Fvector& v, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire);

    virtual void __stdcall DrawGrid	();
    virtual void __stdcall DrawPivot	(const Fvector& pos, float sz=5.f);
	virtual void __stdcall DrawAxis	(const Fmatrix& T);
	virtual void __stdcall DrawObjectAxis(const Fmatrix& T, float sz, BOOL sel);
	virtual void __stdcall DrawSelectionRect(const Ivector2& m_SelStart, const Ivector2& m_SelEnd);

	virtual void __stdcall DrawIndexedPrimitive	(int prim_type, u32 pc, const Fvector& pos, const Fvector* vb, const u32& vb_size, const u32* ib, const u32& ib_size, const u32& clr_argb, float scale=1.0f){};
    virtual void __stdcall DrawPrimitiveL(D3DPRIMITIVETYPE pt, u32 pc, Fvector* vertices, int vc, u32 color, BOOL bCull, BOOL bCycle);
    virtual void __stdcall DrawPrimitiveTL(D3DPRIMITIVETYPE pt, u32 pc, FVF::TL* vertices, int vc, BOOL bCull, BOOL bCycle);
    virtual void __stdcall DrawPrimitiveLIT(D3DPRIMITIVETYPE pt, u32 pc, FVF::LIT* vertices, int vc, BOOL bCull, BOOL bCycle);

    virtual void __stdcall OutText	(const Fvector& pos, LPCSTR text, u32 color=0xFF000000, u32 shadow_color=0xFF909090);

	virtual void 			OnRender		();
};
extern ECORE_API CDrawUtilities DUImpl;
//----------------------------------------------------
#endif /*_INCDEF_D3DUtils_H_*/


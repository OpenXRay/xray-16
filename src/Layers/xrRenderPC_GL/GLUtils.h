#pragma once
#include "..\..\Include\xrRender\DrawUtils.h"

class ECORE_API CDrawUtilities :
	public CDUInterface
{
public:
	CDrawUtilities() { }

	//----------------------------------------------------
	virtual void __stdcall DrawCross(const Fvector& p, float szx1, float szy1, float szz1, float szx2, float szy2, float szz2, u32 clr, BOOL bRot45 = false) { VERIFY(!"CDrawUtilities::DrawCross not implemented."); };
	virtual void __stdcall DrawCross(const Fvector& p, float sz, u32 clr, BOOL bRot45 = false) { VERIFY(!"CDrawUtilities::DrawCross not implemented."); };
	virtual void __stdcall DrawFlag(const Fvector& p, float heading, float height, float sz, float sz_fl, u32 clr, BOOL bDrawEntity) { VERIFY(!"CDrawUtilities::DrawFlag not implemented."); };
	virtual void __stdcall DrawRomboid(const Fvector& p, float radius, u32 clr) { VERIFY(!"CDrawUtilities::DrawRomboid not implemented."); };
	virtual void __stdcall DrawJoint(const Fvector& p, float radius, u32 clr) { VERIFY(!"CDrawUtilities::DrawJoint not implemented."); };

	virtual void __stdcall DrawSpotLight(const Fvector& p, const Fvector& d, float range, float phi, u32 clr) { VERIFY(!"CDrawUtilities::DrawSpotLight not implemented."); };
	virtual void __stdcall DrawDirectionalLight(const Fvector& p, const Fvector& d, float radius, float range, u32 clr) { VERIFY(!"CDrawUtilities::DrawDirectionalLight not implemented."); };
	virtual void __stdcall DrawPointLight(const Fvector& p, float radius, u32 clr) { VERIFY(!"CDrawUtilities::DrawPointLight not implemented."); };

	virtual void __stdcall DrawSound(const Fvector& p, float radius, u32 clr) { VERIFY(!"CDrawUtilities::DrawSound not implemented."); };
	virtual void __stdcall DrawLineSphere(const Fvector& p, float radius, u32 clr, BOOL bCross) { VERIFY(!"CDrawUtilities::DrawLineSphere not implemented."); };

	virtual void __stdcall dbgDrawPlacement(const Fvector& p, int sz, u32 clr, LPCSTR caption = 0, u32 clr_font = 0xffffffff) { VERIFY(!"CDrawUtilities::dbgDrawPlacement not implemented."); };
	virtual void __stdcall dbgDrawVert(const Fvector& p0, u32 clr, LPCSTR caption = 0) { VERIFY(!"CDrawUtilities::dbgDrawVert not implemented."); };
	virtual void __stdcall dbgDrawEdge(const Fvector& p0, const Fvector& p1, u32 clr, LPCSTR caption = 0) { VERIFY(!"CDrawUtilities::dbgDrawEdge not implemented."); };
	virtual void __stdcall dbgDrawFace(const Fvector& p0, const Fvector& p1, const Fvector& p2, u32 clr, LPCSTR caption = 0) { VERIFY(!"CDrawUtilities::dbgDrawFace not implemented."); };

	virtual void __stdcall DrawFace(const Fvector& p0, const Fvector& p1, const Fvector& p2, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) { VERIFY(!"CDrawUtilities::DrawFace not implemented."); };
	virtual void __stdcall DrawLine(const Fvector& p0, const Fvector& p1, u32 clr) { VERIFY(!"CDrawUtilities::DrawLine not implemented."); };
	virtual void __stdcall DrawLink(const Fvector& p0, const Fvector& p1, float sz, u32 clr) { VERIFY(!"CDrawUtilities::DrawLink not implemented."); };
	virtual void __stdcall DrawFaceNormal(const Fvector& p0, const Fvector& p1, const Fvector& p2, float size, u32 clr) { VERIFY(!"CDrawUtilities::DrawFaceNormal not implemented."); };
	virtual void __stdcall DrawFaceNormal(const Fvector* p, float size, u32 clr) { VERIFY(!"CDrawUtilities::DrawFaceNormal not implemented."); };
	virtual void __stdcall DrawFaceNormal(const Fvector& C, const Fvector& N, float size, u32 clr) { VERIFY(!"CDrawUtilities::DrawFaceNormal not implemented."); };
	virtual void __stdcall DrawSelectionBox(const Fvector& center, const Fvector& size, u32* c = 0) { VERIFY(!"CDrawUtilities::DrawSelectionBox not implemented."); };
	virtual void __stdcall DrawSelectionBox(const Fbox& box, u32* c = 0) { VERIFY(!"CDrawUtilities::DrawSelectionBox not implemented."); };
	virtual void __stdcall DrawIdentSphere(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) { VERIFY(!"CDrawUtilities::DrawIdentSphere not implemented."); };
	virtual void __stdcall DrawIdentSpherePart(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) { VERIFY(!"CDrawUtilities::DrawIdentSpherePart not implemented."); };
	virtual void __stdcall DrawIdentCone(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) { VERIFY(!"CDrawUtilities::DrawIdentCone not implemented."); };
	virtual void __stdcall DrawIdentCylinder(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) { VERIFY(!"CDrawUtilities::DrawIdentCylinder not implemented."); };
	virtual void __stdcall DrawIdentBox(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) { VERIFY(!"CDrawUtilities::DrawIdentBox not implemented."); };

	virtual void __stdcall DrawBox(const Fvector& offs, const Fvector& Size, BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) { VERIFY(!"CDrawUtilities::DrawBox not implemented."); };
	virtual void __stdcall DrawAABB(const Fvector& p0, const Fvector& p1, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) { VERIFY(!"CDrawUtilities::DrawAABB not implemented."); };
	virtual void __stdcall DrawAABB(const Fmatrix& parent, const Fvector& center, const Fvector& size, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) { VERIFY(!"CDrawUtilities::DrawAABB not implemented."); };
	virtual void __stdcall DrawOBB(const Fmatrix& parent, const Fobb& box, u32 clr_s, u32 clr_w) { VERIFY(!"CDrawUtilities::DrawOBB not implemented."); };
	virtual void __stdcall DrawSphere(const Fmatrix& parent, const Fvector& center, float radius, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) { VERIFY(!"CDrawUtilities::DrawSphere not implemented."); };
	virtual void __stdcall DrawSphere(const Fmatrix& parent, const Fsphere& S, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) { VERIFY(!"CDrawUtilities::DrawSphere not implemented."); };
	virtual void __stdcall DrawCylinder(const Fmatrix& parent, const Fvector& center, const Fvector& dir, float height, float radius, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) { VERIFY(!"CDrawUtilities::DrawCylinder not implemented."); };
	virtual void __stdcall DrawCone(const Fmatrix& parent, const Fvector& apex, const Fvector& dir, float height, float radius, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) { VERIFY(!"CDrawUtilities::DrawCone not implemented."); };
	virtual void __stdcall DrawPlane(const Fvector& center, const Fvector2& scale, const Fvector& rotate, u32 clr_s, u32 clr_w, BOOL bCull, BOOL bSolid, BOOL bWire) { VERIFY(!"CDrawUtilities::DrawPlane not implemented."); };
	virtual void __stdcall DrawPlane(const Fvector& p, const Fvector& n, const Fvector2& scale, u32 clr_s, u32 clr_w, BOOL bCull, BOOL bSolid, BOOL bWire) { VERIFY(!"CDrawUtilities::DrawPlane not implemented."); };
	virtual void __stdcall DrawRectangle(const Fvector& o, const Fvector& u, const Fvector& v, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) { VERIFY(!"CDrawUtilities::DrawRectangle not implemented."); };

	virtual void __stdcall DrawGrid() { VERIFY(!"CDrawUtilities::DrawGrid not implemented."); };
	virtual void __stdcall DrawPivot(const Fvector& pos, float sz = 5.f) { VERIFY(!"CDrawUtilities::DrawPivot not implemented."); };
	virtual void __stdcall DrawAxis(const Fmatrix& T) { VERIFY(!"CDrawUtilities::DrawAxis not implemented."); };
	virtual void __stdcall DrawObjectAxis(const Fmatrix& T, float sz, BOOL sel) { VERIFY(!"CDrawUtilities::DrawObjectAxis not implemented."); };
	virtual void __stdcall DrawSelectionRect(const Ivector2& m_SelStart, const Ivector2& m_SelEnd) { VERIFY(!"CDrawUtilities::DrawSelectionRect not implemented."); };

	virtual void __stdcall OutText(const Fvector& pos, LPCSTR text, u32 color = 0xFF000000, u32 shadow_color = 0xFF909090) { VERIFY(!"CDrawUtilities::OutText not implemented."); };

	virtual void __stdcall OnDeviceDestroy() { VERIFY(!"CDrawUtilities::OnDeviceDestroy not implemented."); };
};

extern ECORE_API CDrawUtilities DUImpl;

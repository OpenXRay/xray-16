#include "stdafx.h"
#include "glUIRender.h"

#include "glUIShader.h"

glUIRender	UIRenderImpl;

void glUIRender::CreateUIGeom()
{
	hGeom_TL.create(FVF::F_TL, RCache.Vertex.Buffer(), 0);
	hGeom_LIT.create(FVF::F_LIT, RCache.Vertex.Buffer(), 0);
}

void glUIRender::DestroyUIGeom()
{
	hGeom_TL = NULL;
	hGeom_LIT = NULL;
}

void glUIRender::SetShader(IUIShader &shader)
{
	glUIShader *pShader = (glUIShader*)&shader;
	VERIFY(&pShader);
	VERIFY(pShader->hShader);
	RCache.set_Shader(pShader->hShader);
}

void glUIRender::SetAlphaRef(int aref)
{
	//CHK_DX(HW.pDevice->SetRenderState(D3DRS_ALPHAREF,aref));
	RCache.set_AlphaRef(aref);
}
/*
void glUIRender::StartTriList(u32 iMaxVerts)
{
VERIFY(PrimitiveType==ptNone);
m_PointType = pttLIT;
m_iMaxVerts = iMaxVerts;
start_pv	= (FVF::LIT*)RCache.Vertex.Lock	(m_iMaxVerts,hGeom_fan.stride(),vOffset);
pv			= start_pv;
PrimitiveType = ptTriList;
}

void glUIRender::FlushTriList()
{
VERIFY(PrimitiveType==ptTriList);
VERIFY(u32(pv-start_pv)<=m_iMaxVerts);

std::ptrdiff_t p_cnt		= (pv-start_pv)/3;
RCache.Vertex.Unlock		(u32(pv-start_pv),hGeom_fan.stride());
RCache.set_Geometry			(hGeom_fan);
if (p_cnt!=0)RCache.Render	(D3DPT_TRIANGLELIST,vOffset,u32(p_cnt));

PrimitiveType = ptNone;
}

void glUIRender::StartTriFan(u32 iMaxVerts)
{
VERIFY(PrimitiveType==ptNone);
m_iMaxVerts = iMaxVerts;
start_pv	= (FVF::LIT*)RCache.Vertex.Lock	(m_iMaxVerts,hGeom_fan.stride(),vOffset);
pv			= start_pv;
PrimitiveType = ptTriFan;
m_PointType	= pttLIT;

}

void glUIRender::FlushTriFan()
{
VERIFY(PrimitiveType==ptTriFan);
VERIFY(u32(pv-start_pv)<=m_iMaxVerts);

std::ptrdiff_t p_cnt		= pv-start_pv;
RCache.Vertex.Unlock		(u32(p_cnt),hGeom_fan.stride());
RCache.set_Geometry	 		(hGeom_fan);
if (p_cnt>2) RCache.Render	(D3DPT_TRIANGLEFAN,vOffset,u32(p_cnt-2));

PrimitiveType = ptNone;
}

void glUIRender::StartTriStrip(u32 iMaxVerts)
{
VERIFY(PrimitiveType==ptNone);
m_iMaxVerts = iMaxVerts;
start_pv	= (FVF::TL*)RCache.Vertex.Lock	(m_iMaxVerts,hGeom_fan.stride(),vOffset);
pv			= start_pv;
PrimitiveType = ptTriStrip;
}

void glUIRender::FlushTriStrip()
{
}


void glUIRender::StartLineStrip(u32 iMaxVerts)
{
VERIFY(PrimitiveType==ptNone);
m_iMaxVerts = iMaxVerts;
start_pv	= (FVF::LIT*)RCache.Vertex.Lock	(m_iMaxVerts,hGeom_fan.stride(),vOffset);
pv			= start_pv;
PrimitiveType = ptLineStrip;
m_PointType = pttLIT;
}

void glUIRender::FlushLineStrip()
{
VERIFY(PrimitiveType==ptLineStrip);
VERIFY(u32(pv-start_pv)<=m_iMaxVerts);

std::ptrdiff_t p_cnt		= pv-start_pv;
RCache.Vertex.Unlock		(u32(p_cnt),hGeom_fan.stride());
RCache.set_Geometry	 		(hGeom_fan);
if (p_cnt>1) RCache.Render	(D3DPT_LINESTRIP,vOffset,u32(p_cnt-1));

PrimitiveType = ptNone;
}

void glUIRender::StartLineList(u32 iMaxVerts)
{
VERIFY(PrimitiveType==ptNone);
m_iMaxVerts = iMaxVerts;
start_pv	= (FVF::LIT*)RCache.Vertex.Lock	(m_iMaxVerts,hGeom_fan.stride(),vOffset);
pv			= start_pv;
PrimitiveType = ptLineList;
}

void glUIRender::FlushLineList()
{
VERIFY(PrimitiveType==ptLineList);
VERIFY(u32(pv-start_pv)<=m_iMaxVerts);

std::ptrdiff_t p_cnt		= pv-start_pv;
RCache.Vertex.Unlock		(u32(p_cnt),hGeom_fan.stride());
RCache.set_Geometry	 		(hGeom_fan);
if (p_cnt>1) RCache.Render	(D3DPT_LINELIST,vOffset,u32(p_cnt)/2);

PrimitiveType = ptNone;
}
*/
void glUIRender::SetScissor(Irect* rect)
{
	RCache.set_Scissor(rect);
}

void glUIRender::GetActiveTextureResolution(Fvector2 &res)
{
	CTexture* T = RCache.get_ActiveTexture(0);
	res.set(float(T->get_Width()), float(T->get_Height()));
}

LPCSTR glUIRender::UpdateShaderName(LPCSTR tex_name, LPCSTR sh_name)
{
	string_path buff;
	if (FS.exist(buff, "$game_textures$", tex_name, ".ogm"))
		return "hud\\movie";
	else
		return sh_name;
}
/*
void glUIRender::PushPoint(float x, float y, u32 c, float u, float v)
{
VERIFY(m_PointType==pttNone);
pv->set(x, y, 0.0f, c, u, v);
++pv;
}
*/
/*
void glUIRender::PushPoint(int x, int y, u32 c, float u, float v)
{
VERIFY(m_PointType==pttNone);
pv->set(x, y, 0, c, u, v);
++pv;
}
*/

void glUIRender::PushPoint(float x, float y, float z, u32 C, float u, float v)
{
	//.	VERIFY(m_PointType==pttLIT);
	switch (m_PointType)
	{
	case pttLIT:
		LIT_pv->set(x, y, z, C, u, v);
		++LIT_pv;
		break;
	case pttTL:
		TL_pv->set(x, y, C, u, v);
		++TL_pv;
		break;
	}
}

void glUIRender::StartPrimitive(u32 iMaxVerts, ePrimitiveType primType, ePointType pointType)
{
	VERIFY(PrimitiveType == ptNone);
	VERIFY(m_PointType == ptNone);
	//.	R_ASSERT(pointType==pttLIT);

	m_iMaxVerts = iMaxVerts;
	PrimitiveType = primType;
	m_PointType = pointType;

	switch (m_PointType)
	{
	case pttLIT:
		LIT_start_pv = (FVF::LIT*)RCache.Vertex.Lock(m_iMaxVerts, hGeom_LIT.stride(), vOffset);
		LIT_pv = LIT_start_pv;
		break;
	case pttTL:
		TL_start_pv = (FVF::TL*)RCache.Vertex.Lock(m_iMaxVerts, hGeom_TL.stride(), vOffset);
		TL_pv = TL_start_pv;
		break;
	}
}

void glUIRender::FlushPrimitive()
{
	u32 primCount = 0;
	_D3DPRIMITIVETYPE d3dPrimType = D3DPT_FORCE_DWORD;
	std::ptrdiff_t p_cnt = 0;

	switch (m_PointType)
	{
	case pttLIT:
		p_cnt = LIT_pv - LIT_start_pv;
		VERIFY(u32(p_cnt) <= m_iMaxVerts);

		RCache.Vertex.Unlock(u32(p_cnt), hGeom_LIT.stride());
		RCache.set_Geometry(hGeom_LIT);
		break;
	case pttTL:
		p_cnt = TL_pv - TL_start_pv;
		VERIFY(u32(p_cnt) <= m_iMaxVerts);

		RCache.Vertex.Unlock(u32(p_cnt), hGeom_TL.stride());
		RCache.set_Geometry(hGeom_TL);
		break;
	default:
		NODEFAULT;
	}

	//	Update data for primitive type
	switch (PrimitiveType)
	{
	case ptTriStrip:
		primCount = (u32)(p_cnt - 2);
		d3dPrimType = D3DPT_TRIANGLESTRIP;
		break;
	case ptTriList:
		primCount = (u32)(p_cnt / 3);
		d3dPrimType = D3DPT_TRIANGLELIST;
		break;
	case ptLineStrip:
		primCount = (u32)(p_cnt - 1);
		d3dPrimType = D3DPT_LINESTRIP;
		break;
	case ptLineList:
		primCount = (u32)(p_cnt / 2);
		d3dPrimType = D3DPT_LINELIST;
		break;
	default:
		NODEFAULT;
	}

	if (primCount>0)
		RCache.Render(d3dPrimType, vOffset, primCount);

	PrimitiveType = ptNone;
	m_PointType = pttNone;
}

void glUIRender::CacheSetXformWorld(const Fmatrix& M)
{
	RCache.set_xform_world(M);
}

void glUIRender::CacheSetCullMode(CullMode m)
{
	RCache.set_CullMode(CULL_NONE + m);
}

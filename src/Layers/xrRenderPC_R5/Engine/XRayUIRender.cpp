#include "pch.h"

XRayUIRender::XRayUIRender()
{
}

XRayUIRender::~XRayUIRender()
{
}

void XRayUIRender::CreateUIGeom()
{
}

void XRayUIRender::DestroyUIGeom()
{

}

void XRayUIRender::SetShader(IUIShader& shader)
{
}

void XRayUIRender::SetAlphaRef(int aref)
{
}

void XRayUIRender::SetScissor(Irect* rect)
{
}

void XRayUIRender::GetActiveTextureResolution(Fvector2& res)
{
}

void XRayUIRender::PushPoint(float x, float y, float z, u32 C, float u, float v)
{

	
}

void XRayUIRender::StartPrimitive(u32 iMaxVerts, ePrimitiveType primType, ePointType pointType)
{

	

	
}

void XRayUIRender::FlushPrimitive()
{
	

	
}

void XRayUIRender::Flush()
{

}

LPCSTR XRayUIRender::UpdateShaderName(LPCSTR tex_name, LPCSTR sh_name)
{
	return LPCSTR();
}

void XRayUIRender::CacheSetXformWorld(const Fmatrix& M)
{
}

void XRayUIRender::CacheSetCullMode(CullMode)
{
}
XRayUIRender GUIRender;
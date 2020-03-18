#pragma once
class XRayUIRender:public IUIRender
{
public:
	XRayUIRender();
	~XRayUIRender();
	virtual void CreateUIGeom();
	virtual void DestroyUIGeom();

	virtual void SetShader(IUIShader &shader);
	virtual void SetAlphaRef(int aref);
	//.	virtual void StartTriList(u32 iMaxVerts);
	//.	virtual void FlushTriList();
	//.	virtual void StartTriFan(u32 iMaxVerts);
	//.	virtual void FlushTriFan();

		//virtual void StartTriStrip(u32 iMaxVerts);
		//virtual void FlushTriStrip();
	//.	virtual void StartLineStrip(u32 iMaxVerts);
	//.	virtual void FlushLineStrip();
	//.	virtual void StartLineList(u32 iMaxVerts);
	//.	virtual void FlushLineList();
	virtual void SetScissor(Irect* rect = NULL);
	virtual void GetActiveTextureResolution(Fvector2 &res);

	//.	virtual void PushPoint(float x, float y, u32 c, float u, float v);
	//.	virtual void PushPoint(int x, int y, u32 c, float u, float v);
	virtual void PushPoint(float x, float y, float z, u32 C, float u, float v);

	virtual void StartPrimitive(u32 iMaxVerts, ePrimitiveType primType, ePointType pointType);
	virtual void FlushPrimitive();
	virtual void Flush();
	virtual LPCSTR	UpdateShaderName(LPCSTR tex_name, LPCSTR sh_name);

	virtual void	CacheSetXformWorld(const Fmatrix& M);
	virtual void	CacheSetCullMode(CullMode);
private:
	
};
extern XRayUIRender GUIRender;
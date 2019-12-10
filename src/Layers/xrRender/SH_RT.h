#ifndef SH_RT_H
#define SH_RT_H
#pragma once

//////////////////////////////////////////////////////////////////////////
class CRT : public xr_resource_named
{
public:
    CRT();
    ~CRT();
    void create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount = 1, bool useUAV = false);
    void destroy();
    void reset_begin();
    void reset_end();
    BOOL valid() { return !!pTexture; }
    bool used_as_depth() const;

public:
#ifdef USE_OGL
    GLuint pRT;
    GLuint pZRT;
    GLenum target;
#else
    ID3DTexture2D* pSurface;
    ID3DRenderTargetView* pRT;
#endif // USE_OGL
#if defined(USE_DX10) || defined(USE_DX11)
    ID3DDepthStencilView* pZRT;

#ifdef USE_DX11
    ID3D11UnorderedAccessView* pUAView;
#endif

#endif //	USE_DX10
    ref_texture pTexture;

    u32 dwWidth;
    u32 dwHeight;
    D3DFORMAT fmt;

    u64 _order;
};

struct resptrcode_crt : public resptr_base<CRT>
{
#ifdef USE_DX11
    void create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount = 1, bool useUAV = false);
#else
    void create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount = 1);
#endif
    void destroy() { _set(nullptr); }
};
typedef resptr_core<CRT, resptrcode_crt> ref_rt;

/*	//	DX10 cut
//////////////////////////////////////////////////////////////////////////
class		CRTC	:	public xr_resource_named	{
public:
    IDirect3DCubeTexture9*	pSurface;
    IDirect3DSurface9*		pRT[6];
    ref_texture				pTexture;

    u32						dwSize;
    D3DFORMAT				fmt;

    u64						_order;

    CRTC					();
    ~CRTC					();

    void				create			(LPCSTR name, u32 size, D3DFORMAT f);
    void				destroy			();
    void				reset_begin		();
    void				reset_end		();
    IC BOOL				valid			()	{ return !pTexture; }
};
struct 		resptrcode_crtc	: public resptr_base<CRTC>
{
    void				create			(LPCSTR Name, u32 size, D3DFORMAT f);
    void				destroy			()	{ _set(NULL);		}
};
typedef	resptr_core<CRTC,resptrcode_crtc>		ref_rtc;
*/

#endif // SH_RT_H

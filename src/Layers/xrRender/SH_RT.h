#ifndef SH_RT_H
#define SH_RT_H
#pragma once

//////////////////////////////////////////////////////////////////////////
class CRT : public xr_resource_named
{
public:
    enum : u32 // extends xr_resource_flagged flags
    {
        /*RF_REGISTERED = xr_resource_flagged::RF_REGISTERED,*/
        CreateUAV = 1 << 1, // Self descriptive. DX11-specific.
        CreateSurface = 1 << 2, // Creates depth-stencil or offscreen plain surface instead of texture. DX9-specific.
        CreateBase = 1 << 3, // Creates basic RTV from backbuffer or DSV (depending on format)
    };

    CRT();
    ~CRT();
    void create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount = 1, Flags32 flags = {});
    void destroy();
    void reset_begin();
    void reset_end();
    BOOL valid() { return !!pTexture; }
    bool used_as_depth() const;

    void resolve_into(CRT& destination) const; // only RTs with same format supported

public:
#if defined(USE_DX9) || (USE_DX11)
    ID3DTexture2D* pSurface;
    ID3DRenderTargetView* pRT;
#if defined(USE_DX11)
    ID3DDepthStencilView* pZRT;
    ID3D11UnorderedAccessView* pUAView;
#endif
#elif defined(USE_OGL)
    GLuint pRT;
    GLuint pZRT;
    GLenum target;
#else
#	error No graphics API selected or enabled!
#endif

    ref_texture pTexture;

    u32 dwWidth;
    u32 dwHeight;
    D3DFORMAT fmt;
    u32 sampleCount;

    u64 _order;
};

struct resptrcode_crt : public resptr_base<CRT>
{
    void create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount = 1, Flags32 flags = {});
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

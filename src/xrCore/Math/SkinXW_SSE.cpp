#include "stdafx.h"
#include "SkinXW_SSE.hpp"
#ifdef _EDITOR
#include "SkeletonX.h"
#include "SkeletonCustom.h"
#else
#include "Animation/Bone.hpp"
#include "Layers/xrRender/SkeletonXVertRender.h"
#endif

namespace XRay
{
namespace Math
{
#if defined(WINDOWS) && defined(XR_X86)
#define transform_dir(idx, res, SX, SY, SZ, T1) \
    \
__asm movzx eax,                                \
        WORD PTR[esi][idx * (TYPE u16)] S.m \
__asm movaps res,                               \
        SX \
__asm sal eax,                                  \
        5 \
__asm lea eax,                                  \
        [eax + eax * 4] \
__asm movaps T1,                                \
        SY \
__asm mulps res,                                \
        XMMWORD PTR[edx][eax][64] \
__asm mulps T1,                                 \
        XMMWORD PTR[edx][eax][80] \
__asm addps res,                                \
        T1 \
__asm movaps T1,                                \
        SZ \
__asm mulps T1,                                 \
        XMMWORD PTR[edx][eax][96] \
__asm addps res,                                \
        T1

#define transform_tiny(idx, res, SX, SY, SZ, T1) \
    \
transform_dir(idx, res, SX, SY, SZ, T1) \
__asm addps res,                                 \
        XMMWORD PTR[edx][eax][112]

#define shuffle_vec(VEC, SX, SY, SZ) \
    \
__asm movss SX,                      \
        DWORD PTR[esi] VEC.x \
__asm movss SY,                      \
        DWORD PTR[esi] VEC.y \
__asm shufps SX,                     \
        SX, _MM_SHUFFLE(1, 0, 0, 0) \
__asm movss SZ,                      \
        DWORD PTR[esi] VEC.z \
__asm shufps SY,                     \
        SY, _MM_SHUFFLE(1, 0, 0, 0) \
__asm shufps SZ,                     \
        SZ, _MM_SHUFFLE(1, 0, 0, 0)

#define shuffle_sw4(SW0, SW1, SW2, SW3) \
    \
__asm movss SW3,                        \
        DWORD PTR[One] \
__asm movss SW0,                        \
        DWORD PTR[esi][0 * (TYPE float)] S.w \
__asm movss SW1,                        \
        DWORD PTR[esi][1 * (TYPE float)] S.w \
__asm subss SW3,                        \
        SW0 \
__asm shufps SW0,                       \
        SW0, _MM_SHUFFLE(1, 0, 0, 0) \
__asm subss SW3,                        \
        SW1 \
__asm movss SW2,                        \
        DWORD PTR[esi][2 * (TYPE float)] S.w \
__asm shufps SW1,                       \
        SW1, _MM_SHUFFLE(1, 0, 0, 0) \
__asm subss SW3,                        \
        SW2 \
__asm shufps SW2,                       \
        SW2, _MM_SHUFFLE(1, 0, 0, 0) \
__asm shufps SW3,                       \
        SW3, _MM_SHUFFLE(1, 0, 0, 0)

void Skin4W_SSE(vertRender* D, vertBoned4W* S, u32 vCount, CBoneInstance* Bones)
{
    __m128 P0, P1, P2, P3;
    DWORD One;
    __asm {
        // ------------------------------------------------------------------
	mov			edi, DWORD PTR [D]			; edi = D
	mov			esi, DWORD PTR [S]			; esi = S
	mov			ecx, DWORD PTR [vCount]		; ecx = vCount
	mov			edx, DWORD PTR [Bones]		; edx = Bones
	mov			DWORD PTR [One], 0x3f800000	; One = 1.0f
        // ------------------------------------------------------------------
	ALIGN		16				;
	new_vert:					; _new cycle iteration
        // ------------------------------------------------------------------
	shuffle_vec(S.P,xmm4,xmm5,xmm6);

	transform_tiny(0,xmm0,xmm4,xmm5,xmm6,xmm7);		xmm0 = P0
	transform_tiny(1,xmm1,xmm4,xmm5,xmm6,xmm7);		xmm1 = P1
	transform_tiny(2,xmm2,xmm4,xmm5,xmm6,xmm7);		xmm2 = P2
	transform_tiny(3,xmm3,xmm4,xmm5,xmm6,xmm7);		xmm3 = P3

	movaps		XMMWORD PTR [P0], xmm0
	movaps		XMMWORD PTR [P1], xmm1
	movaps		XMMWORD PTR [P2], xmm2
	movaps		XMMWORD PTR [P3], xmm3

	shuffle_vec(S.N,xmm4,xmm5,xmm6);

	transform_dir(0,xmm0,xmm4,xmm5,xmm6,xmm7);		xmm0 = N0
	transform_dir(1,xmm1,xmm4,xmm5,xmm6,xmm7);		xmm1 = N1
	transform_dir(2,xmm2,xmm4,xmm5,xmm6,xmm7);		xmm2 = N2
	transform_dir(3,xmm3,xmm4,xmm5,xmm6,xmm7);		xmm3 = N3

	shuffle_sw4(xmm4,xmm5,xmm6,xmm7);

	mulps		xmm0, xmm4					; xmm0 = N0
	mulps		xmm1, xmm5					; xmm1 = N1
	mulps		xmm2, xmm6					; xmm2 = N2
	addps		xmm0, xmm1					; xmm0 = N0 + N1
	mulps		xmm3, xmm7					; xmm3 = N3

	mulps		xmm4, XMMWORD PTR [P0]		; xmm4 = P0
	mulps		xmm5, XMMWORD PTR [P1]		; xmm5 = P1 
	mulps		xmm6, XMMWORD PTR [P2]		; xmm6 = P2
	addps		xmm4, xmm5					; xmm4 = P0 + P1
	mulps		xmm7, XMMWORD PTR [P3]		; xmm7 = P3

	addps		xmm0, xmm2					; xmm0 = N0 + N1 + N2
	addps		xmm4, xmm6					; xmm4 = P0 + P1 + P2
	addps		xmm0, xmm3					; xmm0 = N0 + N1 + N2 + N3 = 00 | Nz | Ny | Nx
	addps		xmm4, xmm7					; xmm4 = P0 + P1 + P2 + P3 = 00 | Pz | Py | Px
            // ------------------------------------------------------------------
	movlps		xmm1, MMWORD PTR [esi]S.u	; xmm1 = ?? | ?? | v | u
	movaps		xmm5, xmm4					; xmm5 = 00 | Pz | Py | Px
	add			edi,TYPE vertRender			; // advance dest
	movss		xmm5, xmm0					; xmm5 = 00 | Pz | Py | Nx
	prefetchnta BYTE PTR [esi+4*(TYPE vertBoned4W)];		one cache line ahead
	add			esi,TYPE vertBoned4W		; // advance source

	shufps		xmm4, xmm5, _MM_SHUFFLE(0,2,1,0)	; xmm4 = Nx | Pz | Py | Px
	shufps		xmm0, xmm1, _MM_SHUFFLE(1,0,2,1)	; xmm0 = v | u | Nz | Ny
            // ------------------------------------------------------------------
	dec			ecx								; // vCount--
        // ------------------------------------------------------------------
        //	writing data
        // ------------------------------------------------------------------
	movntps		XMMWORD PTR [edi-(TYPE vertRender)],xmm4		; 
	movntps		XMMWORD PTR [edi+16-(TYPE vertRender)],xmm0		;
        // ------------------------------------------------------------------
	jnz			new_vert						; // vCount == 0 ? exit : goto new_vert
        // ------------------------------------------------------------------
	sfence										;	write back cache
        // ------------------------------------------------------------------
    }
}

#define shuffle_sw3(SW0, SW1, SW2) \
    \
__asm movss SW2,                   \
        DWORD PTR[One] \
__asm movss SW0,                   \
        DWORD PTR[esi][0 * (TYPE float)] S.w \
__asm movss SW1,                   \
        DWORD PTR[esi][1 * (TYPE float)] S.w \
__asm subss SW2,                   \
        SW0 \
__asm shufps SW0,                  \
        SW0, _MM_SHUFFLE(1, 0, 0, 0) \
__asm subss SW2,                   \
        SW1 \
__asm shufps SW1,                  \
        SW1, _MM_SHUFFLE(1, 0, 0, 0) \
__asm shufps SW2,                  \
        SW2, _MM_SHUFFLE(1, 0, 0, 0)

void Skin3W_SSE(vertRender* D, vertBoned3W* S, u32 vCount, CBoneInstance* Bones)
{
    __m128 P0, P1;
    DWORD One;
    __asm {
        // ------------------------------------------------------------------
	mov			edi, DWORD PTR [D]			; edi = D
	mov			esi, DWORD PTR [S]			; esi = S
	mov			ecx, DWORD PTR [vCount]		; ecx = vCount
	mov			edx, DWORD PTR [Bones]		; edx = Bones
	mov			DWORD PTR [One], 0x3f800000	; One = 1.0f
        // ------------------------------------------------------------------
	ALIGN		16				;
	new_vert:					; _new cycle iteration
        // ------------------------------------------------------------------
	shuffle_vec(S.P,xmm4,xmm5,xmm6);

	transform_tiny(0,xmm0,xmm4,xmm5,xmm6,xmm7);		xmm0 = P0
	transform_tiny(1,xmm1,xmm4,xmm5,xmm6,xmm7);		xmm1 = P1
	transform_tiny(2,xmm2,xmm4,xmm5,xmm6,xmm7);		xmm2 = P2

	movaps		XMMWORD PTR [P0], xmm0
	movaps		XMMWORD PTR [P1], xmm1

	shuffle_vec(S.N,xmm4,xmm5,xmm6);

	transform_dir(0,xmm0,xmm4,xmm5,xmm6,xmm7);		xmm0 = N0
	transform_dir(1,xmm1,xmm4,xmm5,xmm6,xmm7);		xmm1 = N1
	transform_dir(2,xmm3,xmm4,xmm5,xmm6,xmm7);		xmm3 = N2

	shuffle_sw3(xmm4,xmm5,xmm6);

	mulps		xmm0, xmm4					; xmm0 = N0
	mulps		xmm1, xmm5					; xmm1 = N1
	mulps		xmm3, xmm6					; xmm2 = N2
	
	addps		xmm0, xmm1					; xmm0 = N0 + N1

	mulps		xmm4, XMMWORD PTR [P0]		; xmm4 = P0
	mulps		xmm5, XMMWORD PTR [P1]		; xmm5 = P1 
	mulps		xmm6, xmm2					; xmm6 = P2

	addps		xmm4, xmm5					; xmm4 = P0 + P1

	addps		xmm0, xmm3					; xmm0 = N0 + N1 + N2
	addps		xmm4, xmm6					; xmm4 = P0 + P1 + P2
            // ------------------------------------------------------------------
	movlps		xmm1, MMWORD PTR [esi]S.u	; xmm1 = ?? | ?? | v | u
	movaps		xmm5, xmm4					; xmm5 = 00 | Pz | Py | Px
	add			edi,TYPE vertRender			; // advance dest
	movss		xmm5, xmm0					; xmm5 = 00 | Pz | Py | Nx
	prefetchnta BYTE PTR [esi+8*(TYPE vertBoned3W)];		one cache line ahead
	add			esi,TYPE vertBoned3W		; // advance source

	shufps		xmm4, xmm5, _MM_SHUFFLE(0,2,1,0)	; xmm4 = Nx | Pz | Py | Px
	shufps		xmm0, xmm1, _MM_SHUFFLE(1,0,2,1)	; xmm0 = v | u | Nz | Ny
            // ------------------------------------------------------------------
	dec			ecx								; // vCount--
        // ------------------------------------------------------------------
        //	writing data
        // ------------------------------------------------------------------
	movntps		XMMWORD PTR [edi-(TYPE vertRender)],xmm4		; 
	movntps		XMMWORD PTR [edi+16-(TYPE vertRender)],xmm0		;
        // ------------------------------------------------------------------
	jnz			new_vert						; // vCount == 0 ? exit : goto new_vert
        // ------------------------------------------------------------------
	sfence										;	write back cache
        // ------------------------------------------------------------------
    }
}

#define transform_dir2(idx, res, SX, SY, SZ, T1) \
    \
__asm movzx eax,                                 \
        WORD PTR[esi] S.matrix##idx \
__asm movaps res,                                \
        SX \
__asm sal eax,                                   \
        5 \
__asm lea eax,                                   \
        [eax + eax * 4] \
__asm movaps T1,                                 \
        SY \
__asm mulps res,                                 \
        XMMWORD PTR[edx][eax][64] \
__asm mulps T1,                                  \
        XMMWORD PTR[edx][eax][80] \
__asm addps res,                                 \
        T1 \
__asm movaps T1,                                 \
        SZ \
__asm mulps T1,                                  \
        XMMWORD PTR[edx][eax][96] \
__asm addps res,                                 \
        T1

#define transform_tiny2(idx, res, SX, SY, SZ, T1) \
    \
transform_dir2(idx, res, SX, SY, SZ, T1) \
__asm addps res,                                  \
        XMMWORD PTR[edx][eax][112]

void Skin2W_SSE(vertRender* D, vertBoned2W* S, u32 vCount, CBoneInstance* Bones)
{
    __asm {
        // ------------------------------------------------------------------
	mov			edi, DWORD PTR [D]			; edi = D
	mov			esi, DWORD PTR [S]			; esi = S
	mov			ecx, DWORD PTR [vCount]		; ecx = vCount
	mov			edx, DWORD PTR [Bones]		; edx = Bones
        // ------------------------------------------------------------------
	ALIGN		16				;
	new_vert:					; _new cycle iteration
        // ------------------------------------------------------------------
	shuffle_vec(S.P,xmm4,xmm5,xmm6);

	transform_tiny2(0,xmm0,xmm4,xmm5,xmm6,xmm7);		xmm0 = P0
	transform_tiny2(1,xmm1,xmm4,xmm5,xmm6,xmm7);		xmm1 = P1

	shuffle_vec(S.N,xmm4,xmm5,xmm6);

	transform_dir2(0,xmm2,xmm4,xmm5,xmm6,xmm7);			xmm2 = N0
	transform_dir2(1,xmm3,xmm4,xmm5,xmm6,xmm7);			xmm3 = N1

	movss		xmm7, DWORD PTR [esi]S.w				; xmm7 = 0 | 0 | 0 | w

	subps		xmm1, xmm0								; xmm1 = P1 - P0
	shufps		xmm7, xmm7, _MM_SHUFFLE(1,0,0,0)		; xmm7 = 0 | w | w | w
	subps		xmm3, xmm2								; xmm3 = N1 - N0

	mulps		xmm1, xmm7								; xmm1 = (P1 - P0)*w
	mulps		xmm3, xmm7								; xmm3 = (N1 - N0)*w

	addps		xmm0, xmm1								; xmm0 = P0 + (P1 - P0)*w
	addps		xmm2, xmm3								; xmm2 = N0 + (N1 - N0)*w

	movlps		xmm7, MMWORD PTR [esi]S.u				; xmm7 = ?? | ?? | v | u
	movaps		xmm5, xmm0								; xmm5 = 00 | Pz | Py | Px
	add			edi,TYPE vertRender						; // advance dest
	movss		xmm5, xmm2								; xmm5 = 00 | Pz | Py | Nx
	prefetchnta BYTE PTR [esi+12*(TYPE vertBoned2W)];		one cache line ahead
	add			esi,TYPE vertBoned2W					; // advance source

	shufps		xmm0, xmm5, _MM_SHUFFLE(0,2,1,0)		; xmm0 = Nx | Pz | Py | Px
	shufps		xmm2, xmm7, _MM_SHUFFLE(1,0,2,1)		; xmm2 = v | u | Nz | Ny
            // ------------------------------------------------------------------
	dec			ecx										; // vCount--
        // ------------------------------------------------------------------
        //	writing data
        // ------------------------------------------------------------------
	movntps		XMMWORD PTR [edi-(TYPE vertRender)],xmm0		; 
	movntps		XMMWORD PTR [edi+16-(TYPE vertRender)],xmm2		;
        // ------------------------------------------------------------------
	jnz			new_vert						; // vCount == 0 ? exit : goto new_vert
        // ------------------------------------------------------------------
	sfence										;	write back cache
        // ------------------------------------------------------------------
    }
}

void Skin1W_SSE(vertRender* D, vertBoned1W* S, u32 vCount, CBoneInstance* Bones)
{
    __asm {
        // ------------------------------------------------------------------
	mov			edi, DWORD PTR [D]			; edi = D
	mov			esi, DWORD PTR [S]			; esi = S
	mov			ecx, DWORD PTR [vCount]		; ecx = vCount
	mov			edx, DWORD PTR [Bones]		; edx = Bones
        // ------------------------------------------------------------------
	ALIGN		16				;
	new_vert:					; _new cycle iteration
        // ------------------------------------------------------------------
	mov			eax, DWORD PTR [esi]S.matrix		;	eax = S.matrix

	movss		xmm0, DWORD PTR [esi]S.P.x
	movss		xmm1, DWORD PTR [esi]S.P.y
	lea			eax, [eax+eax*4]					;	eax *= 5 (160)
	shufps		xmm0, xmm0, _MM_SHUFFLE(1,0,0,0)
	movss		xmm2, DWORD PTR [esi]S.P.z
	shufps		xmm1, xmm1, _MM_SHUFFLE(1,0,0,0)
	shufps		xmm2, xmm2, _MM_SHUFFLE(1,0,0,0)

	movss		xmm3, DWORD PTR [esi]S.N.x
	movss		xmm4, DWORD PTR [esi]S.N.y
	sal			eax, 5								;	eax *= 32
	shufps		xmm3, xmm3, _MM_SHUFFLE(1,0,0,0)
	movss		xmm5, DWORD PTR [esi]S.N.z
	shufps		xmm4, xmm4, _MM_SHUFFLE(1,0,0,0)
	shufps		xmm5, xmm5, _MM_SHUFFLE(1,0,0,0)

	mulps		xmm0, XMMWORD PTR [edx][eax][64]
	mulps		xmm1, XMMWORD PTR [edx][eax][80]
	mulps		xmm2, XMMWORD PTR [edx][eax][96]

	mulps		xmm3, XMMWORD PTR [edx][eax][64]
	mulps		xmm4, XMMWORD PTR [edx][eax][80]
	mulps		xmm5, XMMWORD PTR [edx][eax][96]

	addps		xmm0, xmm1
	addps		xmm3, xmm4

	addps		xmm0, xmm2
	addps		xmm3, xmm5

	addps		xmm0, XMMWORD PTR [edx][eax][112]

	movlps		xmm1, MMWORD PTR [esi]S.u				; xmm1 = ?? | ?? | v | u
	movaps		xmm4, xmm0								; xmm4 = 00 | Pz | Py | Px
	add			edi,TYPE vertRender						; // advance dest
	movss		xmm4, xmm3								; xmm4 = 00 | Pz | Py | Nx
	prefetchnta BYTE PTR [esi+16*(TYPE vertBoned1W)];		one cache line ahead
	add			esi,TYPE vertBoned1W					; // advance source

	shufps		xmm0, xmm4, _MM_SHUFFLE(0,2,1,0)		; xmm0 = Nx | Pz | Py | Px
	shufps		xmm3, xmm1, _MM_SHUFFLE(1,0,2,1)		; xmm3 = v | u | Nz | Ny
            // ------------------------------------------------------------------
	dec			ecx								; // vCount--
        // ------------------------------------------------------------------
        //	writing data
        // ------------------------------------------------------------------
	movntps		XMMWORD PTR [edi-(TYPE vertRender)],xmm0		; 
	movntps		XMMWORD PTR [edi+16-(TYPE vertRender)],xmm3		;
        // ------------------------------------------------------------------
	jnz			new_vert						; // vCount == 0 ? exit : goto new_vert
        // ------------------------------------------------------------------
	sfence										;	write back cache
        // ------------------------------------------------------------------
    }
}
#endif
} // namespace Math
} // namespace XRay

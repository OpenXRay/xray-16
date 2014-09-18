// -- cool defines --------------------------------------------------
#define _MANOWAR_SUPER_PROGRAMMER_
#define _TOP_SECRET_
#define _COOL_
#define _AMD_RULEZ_FOREVER_
#define _MS_BUGS_OFF_

// --  includes -----------------------------------------------------
#include "stdafx.h"
#include "..\skeletonanimated.h"

//;******************************************************************************
// A "magic" type to allow initialization with any type
template <class T>
union __m64t
{
    T       t;
    __m64   m64;

    operator __m64 ()       { return m64; }
    operator __m64 () const { return m64; }
};
// An instantiation for initializing with a pair of integers
typedef __m64t<int[2]> __m64i;
// An instantiation for initializing with a pair of floats
typedef __m64t<float[2]> __m64f;
// --  externs from 3DNow!Lib ---------------------------------------
extern void a_acos		();
extern void alt_acos	();
extern void SINCOSMAC	();
// -- consts --------------------------------------------------------
__declspec(align(32)) static const __int64	_msgn_		=	0x8000000080000000;
__declspec(align(32)) static const float	_QEPSILON_	=	0.00001f;
__declspec(align(32)) static const float	_ONE_		=	1.0f;
__declspec(align(32)) static const __m64f	_KEY_Q		=	{1.0f/32767.0f,1.0f/32767.0f};
// -- xrBoneLerp_3DNow ----------------------------------------------
void __stdcall xrBoneLerp_3DNow(	CKey* D,
									CKeyQ* K1,
									CKeyQ* K2,
									float delta)
{ 
// -- local variables -----------------------------------------------
__declspec(align(32)) static __int64	Q0_1;		// Q0.y | Q0.x
__declspec(align(32)) static __int64	Q0_2;		// Q0.w | Q0.z
__declspec(align(32)) static __int64	Q1_1;		// Q1.y | Q1.x
__declspec(align(32)) static __int64	Q1_2;		// Q1.w | Q1.z
__declspec(align(32)) static __int64	omega;		// omega
__declspec(align(32)) static __int64	rev_sinom;	// 1/sinom
__declspec(align(32)) static __int64	tomega;		// 1/sinom
__declspec(align(32)) static __int64	scale0;		// scale0
// -- asm -----------------------------------------------------------
__asm{
// ------------------------------------------------------------------
	femms									; Clear MMX/3DNow! state
// -- targets -------------------------------------------------------
	mov			esi,DWORD PTR [K1]			;
	mov			edi,DWORD PTR [K2]			;
// -- KEY_QuantI ----------------------------------------------------
	movq		mm3,QWORD PTR [_KEY_Q]		; mm3 = Quanti | Quanti
// -- Q0 ------------------------------------------------------------
	movq		mm0,QWORD PTR [esi]K1.x		; mm0 = K1.w | K1.z | K1.y | K1.x
	movq		mm2,mm0						; mm2 = K1.w | K1.z | K1.y | K1.x

	punpcklwd	mm0,mm0						; mm0 = K1.y | K1.y | K1.x | K1.x
	punpckhwd	mm2,mm2						; mm2 = K1.w | K1.w | K1.z | K1.z

	pi2fw		mm0,mm0						; mm0 = K1.y(f) | K1.x(f)
	pi2fw		mm2,mm2						; mm0 = K1.w(f) | K1.z(f)

	pfmul		mm0,mm3						; mm0 = K1.y(f)*KEY_QuantI | K1.x(f)*KEY_QuantI
											; mm0 = Q0.y | Q0.x
	pfmul		mm2,mm3						; mm2 = K1.w(f)*KEY_QuantI | K1.z(f)*KEY_QuantI
											; mm2 = Q0.w | Q0.z
// -- Q1 ------------------------------------------------------------
	movq		mm1,QWORD PTR [edi]K2.x		; mm1 = K2.w | K2.z | K2.y | K2.x
	movq		mm4,mm1						; mm4 = K2.w | K2.z | K2.y | K2.x

	punpcklwd	mm1,mm1						; mm1 = K2.y | K2.y | K2.x | K2.x
	punpckhwd	mm4,mm4						; mm4 = K2.w | K2.w | K2.z | K2.z

	pi2fw		mm1,mm1						; mm1 = K2.y(f) | K2.x(f)
	pi2fw		mm4,mm4						; mm1 = K2.w(f) | K2.z(f)

	pfmul		mm1,mm3						; mm1 = K2.y(f)*KEY_QuantI | K2.x(f)*KEY_QuantI
											; mm1 = Q1.y | Q1.x
	pfmul		mm4,mm3						; mm4 = K2.w(f)*KEY_QuantI | K2.z(f)*KEY_QuantI
											; mm4 = Q1.w | Q1.z
// -- saving mm0 & mm2 ----------------------------------------------
	movq		mm3,mm0						; mm0 = Q0.y | Q0.x
	movq		mm5,mm2						; mm5 = Q0.w | Q0.z
// -- cosom ---------------------------------------------------------
	pfmul		mm0,mm1						; mm0 = Q0.y*Q1.y | Q0.x*Q1.x
	pfmul		mm2,mm4						; mm2 = Q0.w*Q1.w | Q0.z*Q1.z
	pfadd		mm0,mm2						; mm0 = Q0.y*Q1.y + Q0.w*Q1.w | Q0.x*Q1.x + Q0.z*Q1.z
	pfacc		mm0,mm0						; mm0 = Q0.y*Q1.y + Q0.w*Q1.w + Q0.x*Q1.x + Q0.z*Q1.z |
											;		Q0.y*Q1.y + Q0.w*Q1.w | Q0.x*Q1.x + Q0.z*Q1.z
// -- (cosom < 0) ---------------------------------------------------
	pxor		mm2,mm2						; mm2 = 0.0 | 0.0
	pfcmpgt		mm2,mm0						; mm2 = (0 > cosom) ? 0xffffffffh : 0x00000000h
	movd		eax,mm2						; edi = (cosom < 0) ? 0xffffh : 0x0000h
	test		eax,eax						; ZF = (cosom < 0) ? 1 : 0
	jz			cont_comp					;
// -- change signs --------------------------------------------------
	movq		mm2,QWORD PTR [_msgn_]		; mm2 = 0x80000000 | 0x80000000
	pxor		mm0,mm2						; mm0 = -cosom | -cosom
	pxor		mm1,mm2						; mm1 = -Q1.y | -Q1.x			
	pxor		mm4,mm2						; mm4 = -Q1.w | -Q1.z
cont_comp:
// -- (1.0f - cosom) > QEPSILON ) -----------------------------------
	movd		mm2,DWORD PTR [_ONE_]		; mm2 = 0.0 | 1.0
	pfsub		mm2,mm0						; mm2 = 0.0 - cosom | 1.0 -cosom
	movd		mm6,DWORD PTR [_QEPSILON_]	; mm6 = 0.0 | QEPSILON
	pfcmpgt		mm2,mm6						; mm6 = (1.0 - cosom > QEPSILON) ? 0xffffffffh : 0x00000000h
	movd		eax,mm2						; eax = (1.0 - cosom > QEPSILON) ? 0xffffh : 0x0000h
	test		eax,eax						; ZF = (1.0 - cosom > QEPSILON) ? 1 : 0
// -- saving registers into local variables -------------------------
	movq		QWORD PTR [Q0_1],mm3		; Q0_1 = Q0.y | Q0.x
	movq		QWORD PTR [Q0_2],mm5		; Q0_2 = Q0.w | Q0.z
	movq		QWORD PTR [Q1_1],mm1		; Q1_1 = Q1.y | Q1.x
	movq		QWORD PTR [Q1_2],mm4		; Q1_2 = Q1.w | Q1.z
// ------------------------------------------------------------------
	jz			linear_interpol				;
// -- real fuckin' --------------------------------------------------
	call		alt_acos					; mm0 = ?.? | omega=acos(cosom)
	movq		QWORD PTR [omega],mm0		; omega = omega
    call		SINCOSMAC;					; mm0 = cos(x) | sin(x)
    punpckhdq   mm0,mm0						; select sin value
	pfrcp		mm1,mm0						; mm1 = ~ 1/sinom | ~ 1/sinom
	movq		QWORD PTR [rev_sinom],mm1	; rev_sinom = rev_sinom

	movd		mm1,DWORD PTR [delta]		; mm1 = 0.0 | T
	movq		mm0,QWORD PTR [omega]		; mm0 = 0.0 | omega
	pfmul		mm1,mm0						; mm1 = 0.0 | T*omega
	movq		QWORD PTR [tomega],mm1		; tomega = T*omega
	pfsub		mm0,mm1						; mm0 = 0.0 | omega - tomega
	call		SINCOSMAC					; mm0 = sin(omega - tomega) | cos(omega - tomega)
	punpckhdq	mm0,mm0						; mm0 = cos(omega - tomega) | sin(omega - tomega)
	pfmul		mm0,QWORD PTR [rev_sinom]	; mm0 = ?.? | sin(omega - tomega)*rev_sinom
	movq		QWORD PTR [scale0],mm0		; scale0 = sin(omega - tomega)*rev_sinom
	movq		mm0,QWORD PTR [tomega]		; mm0 = 0.0 | tomega
	call		SINCOSMAC					; mm0 = sin(tomega) | cos(tomega)
	punpckhdq	mm0,mm0						; mm0 = cos(tomega) | sin(tomega)
	pfmul		mm0,QWORD PTR [rev_sinom]	; mm0 = ?.? | sin(tomega)*rev_sinom
//	we have scale1 in mm0
// ------------------------------------------------------------------
	jmp short	scale_it					;
// ------------------------------------------------------------------
    ALIGN       16
linear_interpol:
// ------------------------------------------------------------------
	movd		mm7,DWORD PTR [_ONE_]		; mm7 = 0.0 | 1.0
	movd		mm0,DWORD PTR [delta]		; mm0 = 0.0 | T
	pfsub		mm7,mm0						; mm7 = 0.0 | 1.0 - T
	movq		QWORD PTR [scale0],mm7		; scale0 = 1.0 - T
//	we already have scale1 in mm0
// ------------------------------------------------------------------
scale_it:
// -- restoring registers from local variables ----------------------
	movq		mm7,QWORD PTR [Q0_1]		; mm7 = Q0.y | Q0.x
	movq		mm6,QWORD PTR [Q0_2]		; mm6 = Q0.w | Q0.z
	movq		mm5,QWORD PTR [Q1_1]		; mm5 = Q1.y | Q1.x
	movq		mm4,QWORD PTR [Q1_2]		; mm4 = Q1.w | Q1.z
	movq		mm1,QWORD PTR [scale0]		; mm1 = ?.? | scale0
// -- unpacking low -------------------------------------------------
	punpckldq	mm0,mm0						; mm0 = scale1 | scale1
	punpckldq	mm1,mm1						; mm1 = scale0 | scale0
// -- scaling -------------------------------------------------------
	pfmul		mm7,mm1						; mm7 = Q0.y*scale0 | Q0.x*scale0
	pfmul		mm5,mm0						; mm5 = Q1.y*scale1 | Q1.x*scale1

	pfmul		mm6,mm1						; mm6 = Q0.w*scale0 | Q0.z*scale0
	pfmul		mm4,mm0						; mm4 = Q1.w*scale1 | Q1.z*scale1

	pfadd		mm7,mm5						; mm7 = Q0.y*scale0 + Q1.y*scale1 |
											;		Q0.x*scale0 + Q1.x*scale1
	pfadd		mm6,mm4						; mm6 = Q0.w*scale0 + Q1.w*scale1 |
											;		Q0.z*scale0 + Q1.z*scale1
// -- storing results -----------------------------------------------
	mov			edx,DWORD PTR [D]D.Q		;

	movq		QWORD PTR [edx]D.Q.x,mm7	; D.x = Q0.x*scale0 + Q1.x*scale1
											; D.y = Q0.y*scale0 + Q1.y*scale1
	movq		QWORD PTR [edx]D.Q.z,mm6	; D.z = Q0.z*scale0 + Q1.z*scale1
											; D.w = Q0.w*scale0 + Q1.w*scale1
// -- lerp ----------------------------------------------------------
	mov			esi,DWORD PTR [K1]			;
	mov			edi,DWORD PTR [K2]			;
	mov			edx,DWORD PTR [D]			;
	
	movd		mm7,DWORD PTR [_ONE_]		; mm7 = 0.0 | 1.0
	movd		mm6,DWORD PTR [delta]		; mm6 = 0.0 | T

	pfsub		mm7,mm6						; mm7 = 0.0 | 1.0 - T

	movq		mm0,QWORD PTR [esi]K1.t.x	; mm0 = p1.y | p1.x
	movd		mm1,DWORD PTR [esi]K1.t.z	; mm1 = 0.0 | p1.z

	punpckldq	mm6,mm6						; mm6 = T | T
	punpckldq	mm7,mm7						; mm7 = 1.0 - T | 1.0 - T

	movq		mm2,QWORD PTR [edi]K2.t.x	; mm2 = p2.y | p2.x
	movd		mm3,DWORD PTR [edi]K2.t.z	; mm3 = 0.0 | p2.z

	pfmul		mm0,mm7						; mm0 = p1.y*invt | p1.x*invt
	pfmul		mm1,mm7						; mm1 = 0.0 | p1.z*invt

	pfmul		mm2,mm6						; mm2 = p2.y*T | p2.x*T
	pfmul		mm3,mm6						; mm3 =  0.0 | p2.z*T

	pfadd		mm0,mm2						; mm0 = p1.y*invt + p2.y*T |
											;		p1.x*invt + p2.x*T
	pfadd		mm1,mm3						; mm1 = 0.0 | 
											;		p1.z*invt + p2.z*T
	movq		QWORD PTR [edx]D.T.x,mm0	; D.T.x = p1.x*invt + p2.x*T
											; D.T.y = p1.y*invt + p2.y*T
	movd		DWORD PTR [edx]D.T.z,mm1	; D.T.z = p1.z*invt + p2.z*T
// ------------------------------------------------------------------
	femms									; MMX/3DNow! cleanup
// ------------------------------------------------------------------
}}



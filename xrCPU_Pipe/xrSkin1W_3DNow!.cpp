// -- cool defines --------------------------------------------------
#define _MANOWAR_SUPER_PROGRAMMER_
#define _TOP_SECRET_
#define _COOL_
#define _AMD_RULEZ_FOREVER_
#define _MS_BUGS_OFF_
// -- includes ------------------------------------------------------
#include "stdafx.h"
#include "..\skeletonX.h"
#include "..\SkeletonCustom.h"
#pragma warning(disable:4537)
// -- offsets -------------------------------------------------------
#define M11 0
#define M12 4
#define M13 8
#define M14 12
#define M21 16
#define M22 20
#define M23 24
#define M24 28
#define M31 32
#define M32 36
#define M33 40
#define M34 44
#define M41 48
#define M42 52
#define M43 56
#define M44 60
// ------------------------------------------------------------------
void __stdcall xrSkin1W_3DNow(	vertRender*		D,
								vertBoned1W*	S,
								u32				vCount,
								CBoneInstance*	Bones) 
{__asm
{
// ------------------------------------------------------------------
	femms						; Clear MMX/3DNow! state
// ------------------------------------------------------------------
	mov			ecx,vCount		; ecx = vCount ~ 1100
// ------------------------------------------------------------------
	; esi		= source _vector_ [S]
	; edi		= result _vector_ [D]
	; eax		= transform matrix [m]
// ------------------------------------------------------------------
	mov			edi,D			; edi = D
	mov			esi,S			; esi = S
// ------------------------------------------------------------------
	ALIGN		16				;
	new_dot:					; _new cycle iteration
// ------------------------------------------------------------------
// data prefetching
// ------------------------------------------------------------------
	prefetchw	[edi+(TYPE vertRender)*5]		;
	prefetch	[esi+(TYPE vertBoned1W)*5]		;
// ------------------------------------------------------------------
// calculating transformation matrix address
// ------------------------------------------------------------------
	mov			eax,TYPE CBoneInstance			;
	mul			DWORD PTR [esi]S.matrix			;
	add			eax,Bones.mTransform			;
// ------------------------------------------------------------------
// transform tiny
// ------------------------------------------------------------------
			movq		mm0,QWORD PTR [esi]S.P	; mm0 = v.y | v.x
			movd		mm1,DWORD PTR [esi+8]S.P; mm1 = v.z | ?.?
			movq		mm2,mm0					; mm2 = v.y | v.x
			movq		mm3,QWORD PTR [eax+M11]	; mm3 = _12 | _11
			punpckldq	mm0,mm0					; mm0 = v.x | v.x
			movq		mm4,QWORD PTR [eax+M21]	; mm4 = _22 | _21
			pfmul		mm3,mm0					; mm3 = _12*v.x | _11*v.x
			punpckhdq	mm2,mm2					; mm2 = v.y | v.y
			pfmul		mm4,mm2					; mm4 = _22*v.y | _21*v.y
			movq		mm5,QWORD PTR [eax+M13]	; mm5 = _14 | _13
			movq		mm7,QWORD PTR [eax+M23]	; mm7 = _24 | _23
			movq		mm6,mm1					; mm6 = ?.? | v.z
			pfmul		mm5,mm0					; mm5 = _14*v.x | _13*v.x
			movq		mm0,QWORD PTR [eax+M31]	; mm0 = _32 | _31
			punpckldq	mm1,mm1					; mm1 = v.z | v.z
			pfmul		mm7,mm2					; mm7 = _24*v.y | _23*v.y
			movq		mm2,QWORD PTR [eax+M33]	; mm2 = _34 | _33
			pfmul		mm0,mm1					; mm0 = _32*v.z | _31*v.z
			pfadd		mm3,mm4					; mm3 = _12*v.x + _22*v.y |
												;		_11*v.x + _21*v.y
			movq		mm4,QWORD PTR [eax+M41]	; mm4 = _42 | _41
			pfmul		mm2,mm1					; mm2 = _34*v.z | _33*v.z
			pfadd		mm5,mm7					; mm5 = _14*v.x + _24*v.y |
												;		_13*v.x + _23*v.y
			movq		mm1,QWORD PTR [eax+M43]	; mm1 = _44 | _43
			pfadd		mm3,mm0					; mm3 = _12*v.x + _22*v.y + _32*v.z |
												;		_11*v.x + _21*v.y + _31*v.z
			pfadd		mm5,mm2					; _14*v.x + _24*v.y + _34*v.z |
												; _13*v.x + _23*v.y + _33*v.z
			pfadd		mm3,mm4					; _12*v.x + _22*v.y + _32*v.z + _42 |
												; _11*v.x + _21*v.y + _31*v.z + _41
			movq		QWORD PTR [edi]D.P,mm3	; dest.y = _12*v.x + _22*v.y + _32*v.z + _42 
												; dest.x = _11*v.x + _21*v.y + _31*v.z + _41
			pfadd		mm5,mm1					; mm5 = _14*v.x + _24*v.y + _34*v.z + _44 |
												;		_13*v.x + _23*v.y + _33*v.z + _43
			movd		DWORD PTR [edi+8]D.P,mm5; dest.z = _13*v.x + _23*v.y + _33*v.z + _43
// ------------------------------------------------------------------
// transform dir
// ------------------------------------------------------------------	
			movq		mm0,QWORD PTR [esi]S.N	; mm0 = v.y | v.x
			movd		mm1,DWORD PTR [esi+8]S.N; mm1 = ?.? | v.z
			movq		mm2,mm0					; mm2 = v.y | v.x
			movq		mm3,QWORD PTR [eax+M11]	; mm3 = _12 | _11
			punpckldq	mm0,mm0					; mm0 = v.x | v.x
			movq		mm4,QWORD PTR [eax+M21]	; mm4 = _22 | _21
			pfmul		mm3,mm0					; mm3 = _12*v.x | _11*v.x
			punpckhdq	mm2,mm2					; mm2 = v.y | v.y
			pfmul		mm4,mm2					; mm4 = _22*v.y | _21*v.y
			movq		mm5,QWORD PTR [eax+M13]	; mm5 = _14 | _13
			movq		mm7,QWORD PTR [eax+M23]	; mm7 = _24 | _23
			movq		mm6,mm1					; mm6 = ?.? | v.z
			pfmul		mm5,mm0					; mm5 = _14*v.x | _13*v.x
			movq		mm0,QWORD PTR [eax+M31]	; mm0 = _32 | _31
			punpckldq	mm1,mm1					; mm1 = v.z | v.z
			pfmul		mm7,mm2					; mm7 = _24*v.y | _23*v.y
			movq		mm2,QWORD PTR [eax+M33]	; mm2 = _34 | _33
			pfmul		mm0,mm1					; mm0 = _32*v.z | _31*v.z
			pfadd		mm3,mm4					; mm3 = _12*v.x + _22*v.y |
												;		_11*v.x + _21*v.y
			pfmul		mm2,mm1					; mm2 = _34*v.z | _33*v.z
			pfadd		mm5,mm7					; mm5 = _14*v.x + _24*v.y |
												;		_13*v.x + _23*v.y
			pfadd		mm3,mm0					; mm3 = _12*v.x + _22*v.y + _32*v.z |
												;		_11*v.x + _21*v.y + _31*v.z
			movq		QWORD PTR [edi]D.N,mm3	; dest.y = _12*v.x + _22*v.y + _32*v.z
												; dest.x = _11*v.x + _21*v.y + _31*v.z
			pfadd		mm5,mm2					; _14*v.x + _24*v.y + _34*v.z |
												; _13*v.x + _23*v.y + _33*v.z
			movd		DWORD PTR [edi+8]D.N,mm5; dest.z = _13*v.x + _23*v.y + _33*v.z
// ------------------------------------------------------------------
// ------------------------------------------------------------------	
	movq		mm0,[esi]S.u			;
	movq		[edi]D.u,mm0			;
// ------------------------------------------------------------------	
	add			esi,TYPE vertBoned1W	;
	add			edi,TYPE vertRender		;
// ------------------------------------------------------------------
// ------------------------------------------------------------------	
	dec			ecx						; ecx = ecx - 1
	jnz			new_dot					; ecx==0 ? goto new_dot : exit
// ------------------------------------------------------------------
	femms								; Clear MMX/3DNow! state
// ------------------------------------------------------------------
}}

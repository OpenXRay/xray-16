/*
	IC	void	transform_tiny		(Tvector &dest, const Tvector &v)	const // preferred to use
	{
		dest.x = v.x*_11 + v.y*_21 + v.z*_31 + _41;
		dest.y = v.x*_12 + v.y*_22 + v.z*_32 + _42;
		dest.z = v.x*_13 + v.y*_23 + v.z*_33 + _43;
	}
	IC	void	transform_dir		(Tvector &dest, const Tvector &v)	const 	// preferred to use
	{
		dest.x = v.x*_11 + v.y*_21 + v.z*_31;
		dest.y = v.x*_12 + v.y*_22 + v.z*_32;
		dest.z = v.x*_13 + v.y*_23 + v.z*_33;
	}
	IC	SelfRef	lerp(const Self &p1, const Self &p2, T t )
	{
		T invt = 1.f-t;
		x = p1.x*invt + p2.x*t;
		y = p1.y*invt + p2.y*t;
		z = p1.z*invt + p2.z*t;
		return *this;	
	}
	struct vertBoned2W	// (1+3+3 + 1+3+3 + 2)*4 = 16*4 = 64 bytes
	{
		u16	matrix0;
		u16	matrix1;
		Fvector	P0;
		Fvector	N0;
		Fvector	P1;
		Fvector	N1;
		float	w;
		float	u,v;
	};
	struct vertRender
	{
		Fvector	P;
		Fvector	N;
		float	u,v;
	};
*/
#include "stdafx.h"
#pragma hdrstop

#ifdef _EDITOR
#include "skeletonX.h"
#include "skeletoncustom.h"
#else
#include "..\skeletonX.h"
#include "..\skeletoncustom.h"
#endif
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

void __stdcall xrSkin2W_3DNow(	vertRender*		D,
								vertBoned2W*	S,
								u32				vCount,
								CBoneInstance*	Bones) 
{__asm{
// ------------------------------------------------------------------
	mov			ecx,vCount						; ecx = vCount
// ------------------------------------------------------------------
//	esi		= source _vector_ [S]
//	edi		= result _vector_ [D]
//	eax		= transform matrix 0 [m0]
//	ebx		= transform matrix 1 [m1]
// ------------------------------------------------------------------
	mov			edi,D							; edi = D
	mov			esi,S							; esi = S
// ------------------------------------------------------------------
	ALIGN		16								; padding
	new_dot:									; _new cycle iteration
// ------------------------------------------------------------------
// checking whether the matrixes are equal
// ------------------------------------------------------------------
	movzx		eax,WORD PTR [esi]S.matrix0		; eax = matrix0
	movzx		ebx,WORD PTR [esi]S.matrix1		; ebx = matrix1
	cmp			eax,ebx							;
	jz			save_private_case				;
// ------------------------------------------------------------------
// calculating transformation matrix 0 and 1 addresses
// ------------------------------------------------------------------
	lea			eax,[eax+eax*2]					; eax = matrix0*3
	lea			ebx,[ebx+ebx*2]					; ebx = matrix1*3
	shl			eax,5							; eax = matrix0*3*32(96)
	shl			ebx,5							; ebx = matrix1*3*32(96)
	add			eax,Bones						; eax = matrix0*sizeof(CBoneInstance)+Bones
	add			ebx,Bones						; ebx = matrix1*sizeof(CBoneInstance)+Bones
// ------------------------------------------------------------------
// transform tiny m0
// ------------------------------------------------------------------
	movd		mm0,DWORD PTR [esi]S.P0			; mm0 = ? | p0.x
	movd		mm2,DWORD PTR [esi+4]S.P0		; mm2 = ? | p0.y
	movd		mm4,DWORD PTR [esi+8]S.P0		; mm4 = ? | p0.z

	movq		mm1,mm0							; mm1 = p0.x | p0.x
	movq		mm3,mm2							; mm3 = p0.y | p0.y
	movq		mm5,mm4							; mm5 = p0.z | p0.z

	punpckldq	mm0,mm0							; mm0 = p0.x | p0.x
	punpckldq	mm2,mm2							; mm2 = p0.y | p0.y
	punpckldq	mm4,mm4							; mm4 = p0.z | p0.z

	pfmul		mm0,QWORD PTR [eax+M11]			; mm0 = p0.x*_12 | p0.x*_11
	pfmul		mm1,QWORD PTR [eax+M13]			; mm1 = p0.x*_14 | p0.x*_13

	pfmul		mm2,QWORD PTR [eax+M21]			; mm2 = p0.y*_22 | p0.y*_21
	pfmul		mm3,QWORD PTR [eax+M23]			; mm3 = p0.y*_24 | p0.y*_23

	pfmul		mm4,QWORD PTR [eax+M31]			; mm4 = p0.z*_32 | p0.z*_31
	pfmul		mm5,QWORD PTR [eax+M33]			; mm5 = p0.z*_34 | p0.z*_33

	pfadd		mm0,mm2		; mm0 = p0.x*_12 + p0.y*_22 | p0.x*_11 + p0.y*_21
	pfadd		mm1,mm3		; mm1 = p0.x*_14 + p0.y*_24 | p0.x*_13 + p0.y*_23

	pfadd		mm4,QWORD PTR [eax+M41]			; mm4 = p0.z*_32 + _42 | p0.z*_31 + _41
	pfadd		mm5,QWORD PTR [eax+M43]			; mm5 = p0.z*_34 + _44 | p0.z*_33 + _43

	pfadd		mm0,mm4		; mm0 = p0.x*_12 + p0.y*_22 + p0.z*_32 + _42 | p0.x*_11 + p0.y*_21 + p0.z*_31 + _41
	pfadd		mm1,mm5		; mm1 = p0.x*_14 + p0.y*_24 + p0.z*_34 + _44 | p0.x*_13 + p0.y*_23 + p0.z*_33 + _43
// ------------------------------------------------------------------
// => mm0 = P0.y | P0.x
// => mm1 = P0.? | P0.z
// ------------------------------------------------------------------
// transform tiny m1
// ------------------------------------------------------------------
	movd		mm6,DWORD PTR [esi]S.P1			; mm6 = ? | p1.x
	movd		mm2,DWORD PTR [esi+4]S.P1		; mm2 = ? | p1.y
	movd		mm4,DWORD PTR [esi+8]S.P1		; mm4 = ? | p1.z

	movq		mm7,mm6							; mm7 = p1.x | p1.x
	movq		mm3,mm2							; mm3 = p1.y | p1.y
	movq		mm5,mm4							; mm5 = p1.z | p1.z

	punpckldq	mm6,mm6							; mm6 = p1.x | p1.x
	punpckldq	mm2,mm2							; mm2 = p1.y | p1.y
	punpckldq	mm4,mm4							; mm4 = p1.z | p1.z

	pfmul		mm6,QWORD PTR [ebx+M11]			; mm6 = p1.x*_12 | p1.x*_11
	pfmul		mm7,QWORD PTR [ebx+M13]			; mm7 = p1.x*_14 | p1.x*_13

	pfmul		mm2,QWORD PTR [ebx+M21]			; mm2 = p1.y*_22 | p1.y*_21
	pfmul		mm3,QWORD PTR [ebx+M23]			; mm3 = p1.y*_24 | p1.y*_23

	pfmul		mm4,QWORD PTR [ebx+M31]			; mm4 = p1.z*_32 | p1.z*_31
	pfmul		mm5,QWORD PTR [ebx+M33]			; mm5 = p1.z*_34 | p1.z*_33

	pfadd		mm6,mm2		; mm6 = p1.x*_12 + p1.y*_22 | p1.x*_11 + p1.y*_21
	pfadd		mm7,mm3		; mm7 = p1.x*_14 + p1.y*_24 | p1.x*_13 + p1.y*_23

	pfadd		mm4,QWORD PTR [ebx+M41]			; mm4 = p1.z*_32 + _42 | p1.z*_31 + _41
	pfadd		mm5,QWORD PTR [ebx+M43]			; mm5 = p1.z*_34 + _44 | p1.z*_33 + _43

	pfadd		mm6,mm4		; mm6 = p1.x*_12 + p1.y*_22 + p1.z*_32 + _42 | p1.x*_11 + p1.y*_21 + p1.z*_31 + _41
	pfadd		mm7,mm5		; mm7 = p1.x*_14 + p1.y*_24 + p1.z*_34 + _44 | p1.x*_13 + p1.y*_23 + p1.z*_33 + _43
// ------------------------------------------------------------------
// => mm6 = P1.y | P1.x
// => mm7 = P1.? | P1.z
// ------------------------------------------------------------------
// lerp P0 & P1
// ------------------------------------------------------------------
	movd		mm2,DWORD PTR [esi]S.w			; mm2 = 0 | w
	movq		mm3,mm2							; mm3 = w | w
	punpckldq	mm2,mm2							; mm2 = w | w

	pfsub		mm6,mm0							; mm6 = P1.y - P0.y | P1.x - P0.x
	pfsub		mm7,mm1							; mm7 = P1.? - P0.? | P1.z - P0.z

	pfmul		mm6,mm2							; mm6 = (P1.y - P0.y)*w | (P1.x - P0.x)*w
	pfmul		mm7,mm3							; mm7 = (P1.? - P0.?)*w | (P1.z - P0.z)*w

	pfadd		mm6,mm0							; mm6 = (P1.y - P0.y)*w + P0.y | (P1.x - P0.x)*w + P0.x
	pfadd		mm7,mm1							; mm7 = (P1.? - P0.?)*w + P0.? | (P1.z - P0.z)*w + P0.z
// ------------------------------------------------------------------
// => mm6 = DP.y | DP.x
// => mm7 = DP.? | DP.z
// ------------------------------------------------------------------
// storing DP result to memory
// ------------------------------------------------------------------
	movntq		QWORD PTR [edi]D.P,mm6			;
	movd		DWORD PTR [edi+8]D.P,mm7		;
// ------------------------------------------------------------------
// transform dir m0
// ------------------------------------------------------------------
	movd		mm0,DWORD PTR [esi]S.N0			; mm0 = ? | n0.x
	movd		mm2,DWORD PTR [esi+4]S.N0		; mm2 = ? | n0.y
	movd		mm4,DWORD PTR [esi+8]S.N0		; mm4 = ? | n0.z

	movq		mm1,mm0							; mm1 = n0.x | n0.x
	movq		mm3,mm2							; mm3 = n0.y | n0.y
	movq		mm5,mm4							; mm5 = n0.z | n0.z

	punpckldq	mm0,mm0							; mm0 = n0.x | n0.x
	punpckldq	mm2,mm2							; mm2 = n0.y | n0.y
	punpckldq	mm4,mm4							; mm4 = n0.z | n0.z

	pfmul		mm0,QWORD PTR [eax+M11]			; mm0 = n0.x*_12 | n0.x*_11
	pfmul		mm1,QWORD PTR [eax+M13]			; mm1 = n0.x*_14 | n0.x*_13

	pfmul		mm2,QWORD PTR [eax+M21]			; mm2 = n0.y*_22 | n0.y*_21
	pfmul		mm3,QWORD PTR [eax+M23]			; mm3 = n0.y*_24 | n0.y*_23

	pfmul		mm4,QWORD PTR [eax+M31]			; mm4 = n0.z*_32 | n0.z*_31
	pfmul		mm5,QWORD PTR [eax+M33]			; mm5 = n0.z*_34 | n0.z*_33

	pfadd		mm0,mm2		; mm0 = n0.x*_12 + n0.y*_22 | n0.x*_11 + n0.y*_21
	pfadd		mm1,mm3		; mm1 = n0.x*_14 + n0.y*_24 | n0.x*_13 + n0.y*_23

	pfadd		mm0,mm4		; mm0 = n0.x*_12 + n0.y*_22 + n0.z*_32 | n0.x*_11 + n0.y*_21 + n0.z*_31
	pfadd		mm1,mm5		; mm1 = n0.x*_14 + n0.y*_24 + n0.z*_34 | n0.x*_13 + n0.y*_23 + n0.z*_33
// ------------------------------------------------------------------
// => mm0 = N0.y | N0.x
// => mm1 = N0.? | N0.z
// ------------------------------------------------------------------
// transform dir m1
// ------------------------------------------------------------------
	movd		mm6,DWORD PTR [esi]S.N1			; mm6 = ? | n1.x
	movd		mm2,DWORD PTR [esi+4]S.N1		; mm2 = ? | n1.y
	movd		mm4,DWORD PTR [esi+8]S.N1		; mm4 = ? | n1.z

	movq		mm7,mm6							; mm7 = n1.x | n1.x
	movq		mm3,mm2							; mm3 = n1.y | n1.y
	movq		mm5,mm4							; mm5 = n1.z | n1.z

	punpckldq	mm6,mm6							; mm6 = n1.x | n1.x
	punpckldq	mm2,mm2							; mm2 = n1.y | n1.y
	punpckldq	mm4,mm4							; mm4 = n1.z | n1.z

	pfmul		mm6,QWORD PTR [ebx+M11]			; mm6 = n1.x*_12 | n1.x*_11
	pfmul		mm7,QWORD PTR [ebx+M13]			; mm7 = n1.x*_14 | n1.x*_13

	pfmul		mm2,QWORD PTR [ebx+M21]			; mm2 = n1.y*_22 | n1.y*_21
	pfmul		mm3,QWORD PTR [ebx+M23]			; mm3 = n1.y*_24 | n1.y*_23

	pfmul		mm4,QWORD PTR [ebx+M31]			; mm4 = n1.z*_32 | n1.z*_31
	pfmul		mm5,QWORD PTR [ebx+M33]			; mm5 = n1.z*_34 | n1.z*_33

	pfadd		mm6,mm2		; mm6 = n1.x*_12 + n1.y*_22 | n1.x*_11 + n1.y*_21
	pfadd		mm7,mm3		; mm7 = n1.x*_14 + n1.y*_24 | n1.x*_13 + n1.y*_23

	pfadd		mm6,mm4		; mm6 = n1.x*_12 + n1.y*_22 + n1.z*_32 | n1.x*_11 + n1.y*_21 + n1.z*_31
	pfadd		mm7,mm5		; mm7 = n1.x*_14 + n1.y*_24 + n1.z*_34 | n1.x*_13 + n1.y*_23 + n1.z*_33
// ------------------------------------------------------------------
// => mm6 = N1.y | N1.x
// => mm7 = N1.? | N1.z
// ------------------------------------------------------------------
// lerp N0 & N1
// ------------------------------------------------------------------
	movd		mm2,DWORD PTR [esi]S.w			; mm2 = 0 | w
	movq		mm3,mm2							; mm3 = w | w
	punpckldq	mm2,mm2							; mm2 = w | w

	pfsub		mm6,mm0							; mm6 = N1.y - N0.y | N1.x - N0.x
	pfsub		mm7,mm1							; mm7 = N1.? - N0.? | N1.z - N0.z

	pfmul		mm6,mm2							; mm6 = (N1.y - N0.y)*w | (N1.x - N0.x)*w
	pfmul		mm7,mm3							; mm7 = (N1.? - N0.?)*w | (N1.z - N0.z)*w

	pfadd		mm6,mm0							; mm6 = (N1.y - N0.y)*w + N0.y | (N1.x - N0.x)*w + N0.x
	pfadd		mm7,mm1							; mm7 = (N1.? - N0.?)*w + N0.? | (N1.z - N0.z)*w + N0.z
// ------------------------------------------------------------------
// => mm6 = DP.y | DP.x
// => mm7 = DP.? | DP.z
// ------------------------------------------------------------------
// storing DN result to memory
// ------------------------------------------------------------------
	movntq		QWORD PTR [edi]D.N,mm6			;
	movd		DWORD PTR [edi+8]D.N,mm7		;
// ------------------------------------------------------------------
// copying U,V
// ------------------------------------------------------------------
	movq		mm2,QWORD PTR [esi]S.u			; mm2 = v | u
	movntq		QWORD PTR [edi]D.u,mm2			;
// ------------------------------------------------------------------
// advancing pointers	
// ------------------------------------------------------------------	
	add			esi,TYPE vertBoned2W			; esi += sizeof(vertBoned2W)
	add			edi,TYPE vertRender				; edi += sizeof(vertRender)
// ------------------------------------------------------------------	
	dec			ecx								; ecx -= 1
	jnz			new_dot							; ecx == 0 ? goto new_dot : exit
	jmp short	we_are_done						;
// ------------------------------------------------------------------
	ALIGN		16								; padding
	save_private_case:							; private case where matrixes are equal
// ------------------------------------------------------------------
// calculating transformation matrix 0 addresses
// ------------------------------------------------------------------
	lea			eax,[eax+eax*2]					; eax = matrix0*3
	shl			eax,5							; eax = matrix0*3*32(96)
	add			eax,Bones						; eax = matrix0*sizeof(CBoneInstance)+Bones
// ------------------------------------------------------------------
// transform tiny m0
// ------------------------------------------------------------------
	movd		mm0,DWORD PTR [esi]S.P0			; mm0 = ? | p0.x
	movd		mm2,DWORD PTR [esi+4]S.P0		; mm2 = ? | p0.y
	movd		mm4,DWORD PTR [esi+8]S.P0		; mm4 = ? | p0.z

	movq		mm1,mm0							; mm1 = p0.x | p0.x
	movq		mm3,mm2							; mm3 = p0.y | p0.y
	movq		mm5,mm4							; mm5 = p0.z | p0.z

	punpckldq	mm0,mm0							; mm0 = p0.x | p0.x
	punpckldq	mm2,mm2							; mm2 = p0.y | p0.y
	punpckldq	mm4,mm4							; mm4 = p0.z | p0.z

	pfmul		mm0,QWORD PTR [eax+M11]			; mm0 = p0.x*_12 | p0.x*_11
	pfmul		mm1,QWORD PTR [eax+M13]			; mm1 = p0.x*_14 | p0.x*_13

	pfmul		mm2,QWORD PTR [eax+M21]			; mm2 = p0.y*_22 | p0.y*_21
	pfmul		mm3,QWORD PTR [eax+M23]			; mm3 = p0.y*_24 | p0.y*_23

	pfmul		mm4,QWORD PTR [eax+M31]			; mm4 = p0.z*_32 | p0.z*_31
	pfmul		mm5,QWORD PTR [eax+M33]			; mm5 = p0.z*_34 | p0.z*_33

	pfadd		mm0,mm2		; mm0 = p0.x*_12 + p0.y*_22 | p0.x*_11 + p0.y*_21
	pfadd		mm1,mm3		; mm1 = p0.x*_14 + p0.y*_24 | p0.x*_13 + p0.y*_23

	pfadd		mm4,QWORD PTR [eax+M41]			; mm4 = p0.z*_32 + _42 | p0.z*_31 + _41
	pfadd		mm5,QWORD PTR [eax+M43]			; mm5 = p0.z*_34 + _44 | p0.z*_33 + _43

	pfadd		mm0,mm4		; mm0 = p0.x*_12 + p0.y*_22 + p0.z*_32 + _42 | p0.x*_11 + p0.y*_21 + p0.z*_31 + _41
	pfadd		mm1,mm5		; mm1 = p0.x*_14 + p0.y*_24 + p0.z*_34 + _44 | p0.x*_13 + p0.y*_23 + p0.z*_33 + _43
// ------------------------------------------------------------------
// => mm0 = P0.y | P0.x
// => mm1 = P0.? | P0.z
// ------------------------------------------------------------------
// storing DP result to memory
// ------------------------------------------------------------------
	movntq		QWORD PTR [edi]D.P,mm0			;
	movd		DWORD PTR [edi+8]D.P,mm1		;
// ------------------------------------------------------------------
// transform dir m0
// ------------------------------------------------------------------
	movd		mm0,DWORD PTR [esi]S.N0			; mm0 = ? | n0.x
	movd		mm2,DWORD PTR [esi+4]S.N0		; mm2 = ? | n0.y
	movd		mm4,DWORD PTR [esi+8]S.N0		; mm4 = ? | n0.z

	movq		mm1,mm0							; mm1 = n0.x | n0.x
	movq		mm3,mm2							; mm3 = n0.y | n0.y
	movq		mm5,mm4							; mm5 = n0.z | n0.z

	punpckldq	mm0,mm0							; mm0 = n0.x | n0.x
	punpckldq	mm2,mm2							; mm2 = n0.y | n0.y
	punpckldq	mm4,mm4							; mm4 = n0.z | n0.z

	pfmul		mm0,QWORD PTR [eax+M11]			; mm0 = n0.x*_12 | n0.x*_11
	pfmul		mm1,QWORD PTR [eax+M13]			; mm1 = n0.x*_14 | n0.x*_13

	pfmul		mm2,QWORD PTR [eax+M21]			; mm2 = n0.y*_22 | n0.y*_21
	pfmul		mm3,QWORD PTR [eax+M23]			; mm3 = n0.y*_24 | n0.y*_23

	pfmul		mm4,QWORD PTR [eax+M31]			; mm4 = n0.z*_32 | n0.z*_31
	pfmul		mm5,QWORD PTR [eax+M33]			; mm5 = n0.z*_34 | n0.z*_33

	pfadd		mm0,mm2		; mm0 = n0.x*_12 + n0.y*_22 | n0.x*_11 + n0.y*_21
	pfadd		mm1,mm3		; mm1 = n0.x*_14 + n0.y*_24 | n0.x*_13 + n0.y*_23

	pfadd		mm0,mm4		; mm0 = n0.x*_12 + n0.y*_22 + n0.z*_32 | n0.x*_11 + n0.y*_21 + n0.z*_31
	pfadd		mm1,mm5		; mm1 = n0.x*_14 + n0.y*_24 + n0.z*_34 | n0.x*_13 + n0.y*_23 + n0.z*_33
// ------------------------------------------------------------------
// => mm0 = N0.y | N0.x
// => mm1 = N0.? | N0.z
// ------------------------------------------------------------------
// storing DN result to memory
// ------------------------------------------------------------------
	movntq		QWORD PTR [edi]D.N,mm0			;
	movd		DWORD PTR [edi+8]D.N,mm1		;
// ------------------------------------------------------------------
// copying U,V
// ------------------------------------------------------------------
	movq		mm2,QWORD PTR [esi]S.u			; mm2 = v | u
	movntq		QWORD PTR [edi]D.u,mm2			;
// ------------------------------------------------------------------
// advancing pointers	
// ------------------------------------------------------------------	
	add			esi,TYPE vertBoned2W			; esi += sizeof(vertBoned2W)
	add			edi,TYPE vertRender				; edi += sizeof(vertRender)
// ------------------------------------------------------------------	
	dec			ecx								; ecx -= 1
	jnz			new_dot							; ecx == 0 ? goto new_dot : exit
// ------------------------------------------------------------------	
	we_are_done:								; indeed
// ------------------------------------------------------------------
	sfence										; flush caches to slow memory
// ------------------------------------------------------------------
	femms										; reset FPU/3DNow! state
// ------------------------------------------------------------------
}}

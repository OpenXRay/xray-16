#include "stdafx.h"

#define GeomBytes		24	// pos+norm

void	__stdcall	xrTransfer_x86	(LPVOID vDest, LPVOID vSrc, u32 vCount, u32 vStride,
									 LPWORD iDest, LPWORD iSrc, u32 iCount, u32 iOffset,
									 Fmatrix* xform)
{
	// Transfer vertices
	if (xform) 
	{
		LPBYTE	sit		= LPBYTE(vSrc);
		LPBYTE	send	= sit+vCount*vStride;
		LPBYTE	dit		= LPBYTE(vDest);
		DWORD	remain	= vStride-GeomBytes;		// 2fvector of 3 floats

		switch (remain) {
		case 8:		// 32 byte vertex	(pos(12)+norm(12)+uv1(8))
			for (; sit!=send; sit+=vStride, dit+=vStride)
			{
				Fvector*	sP	= (Fvector*)sit;
				Fvector*	dP	= (Fvector*)dit;
				Fvector*	sN	= (Fvector*)(sit+3*4);
				Fvector*	dN	= (Fvector*)(dit+3*4);
				xform->transform_tiny	(*dP,*sP);
				xform->transform_dir	(*dN,*sN);
				CopyMemory	(dit+GeomBytes,sit+GeomBytes,8);
			}
			break;
		case 16:	// 40 byte vertex	(pos(12)+norm(12)+uv1(8)+uv2(8))
			for (; sit!=send; sit+=vStride, dit+=vStride)
			{
				Fvector*	sP	= (Fvector*)sit;
				Fvector*	dP	= (Fvector*)dit;
				Fvector*	sN	= (Fvector*)(sit+3*4);
				Fvector*	dN	= (Fvector*)(dit+3*4);
				xform->transform_tiny	(*dP,*sP);
				xform->transform_dir	(*dN,*sN);
				CopyMemory	(dit+GeomBytes,sit+GeomBytes,16);
			}
			break;
		default:	// any size
			for (; sit!=send; sit+=vStride, dit+=vStride)
			{
				Fvector*	sP	= (Fvector*)sit;
				Fvector*	dP	= (Fvector*)dit;
				Fvector*	sN	= (Fvector*)(sit+3*4);
				Fvector*	dN	= (Fvector*)(dit+3*4);
				xform->transform_tiny	(*dP,*sP);
				xform->transform_dir	(*dN,*sN);
				CopyMemory	(dit+GeomBytes,sit+GeomBytes,remain);
			}
			break;
		}
	} else {
		CopyMemory	(vDest,vSrc,vCount*vStride);
	}

	// Transfer indices (in 32bit lines)
	VERIFY	(iOffset<65535);
	{
		DWORD	item	= (iOffset<<16) | iOffset;
		DWORD	count	= iCount/2;
		LPDWORD	sit		= LPDWORD(iSrc);
		LPDWORD	send	= sit+count;
		LPDWORD	dit		= LPDWORD(iDest);
		for		(; sit!=send; dit++,sit++)	*dit=*sit+item;
		if		(iCount&1)	iDest[iCount-1]=iSrc[iCount-1]+u16(iOffset);
	}
	/*
	{
		DWORD	count	= iCount/(4*3);
		u64*	sit		= (u64*)(iSrc);
		u64*	send	= sit+(count*3);
		u64*	dit		= (u64*)(iDest);
		u64*	dend	= dit+(count*3);

		if (sit!=send)	{
			u64		item	= (u64(iOffset)<<48) | (u64(iOffset)<<32) | (u64(iOffset)<<16) | u64(iOffset);
			__asm	{
				femms		
				movq		mm0,	[item];
				mov			eax,	[sit];
				mov			ebx,	[send];
				mov			edx,	[dit];
				prefetch			[eax+64];
				prefetch			[eax+128];
				prefetch			[eax+196];
loop_start:
				movq		mm1,	[eax];
				movq		mm2,	[eax+4];
				movq		mm3,	[eax+8];
				paddw		mm1,	mm0;
				paddw		mm2,	mm0;
				paddw		mm3,	mm0;
				movq		[edx],	mm1;
				movq		[edx+4],mm2;
				movq		[edx+8],mm3;
				add			eax,	(4*3);
				add			edx,	(4*3);
				cmp			eax,ebx;
				jnz			loop_start;
				femms
			}
		}
		
		DWORD	remain	= iCount-count*(4*3);
		if (remain) {
			LPWORD	s_it	= LPWORD(send);
			LPWORD	s_end	= s_it+remain;
			LPWORD	d_it	= LPWORD(dend);
			LPWORD	d_end	= d_it+remain;
			u16	w_item	= u16(iOffset);
			for (; s_it!=s_end; ) *d_it++ = w_item + *s_it++;
		}
	}
	*/
}

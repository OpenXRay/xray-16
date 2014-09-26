///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains all memory macros.
 *	\file		IceMemoryMacros.h
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __ICEMEMORYMACROS_H__
#define __ICEMEMORYMACROS_H__

	//!	Fills a buffer with a given dword.
	//!	\param		addr	[in] buffer address
	//!	\param		nb		[in] number of dwords to write
	//!	\param		value	[in] the dword value
	//!	\see		FillMemory
	//!	\see		ZeroMemory
	//!	\see		CopyMemory
	//!	\see		MoveMemory
	//!	\warning	writes nb*4 bytes !
/*	inline_ void StoreDwords(udword* dest, udword nb, udword value)
	{
		// The asm code below **SHOULD** be equivalent to one of those C versions
		// or the other if your compiled is good: (checked on VC++ 6.0)
		//
		//	1) while(nb--)	*dest++ = value;
		//
		//	2) for(udword i=0;i<nb;i++)	dest[i] = value;
		//
		_asm push eax
		_asm push ecx
		_asm push edi
		_asm mov edi, dest
		_asm mov ecx, nb
		_asm mov eax, value
		_asm rep stosd
		_asm pop edi
		_asm pop ecx
		_asm pop eax
	}
*/
	#define SIZEOFOBJECT		sizeof(*this)									//!< Gives the size of current object. Avoid some mistakes (e.g. "sizeof(this)").
	//#define CLEAROBJECT		{ memset(this, 0, SIZEOFOBJECT);	}			//!< Clears current object. Laziness is my business. HANDLE WITH CARE.
	#define SAFE_RELEASE(x)		if (x) { (x)->Release();		(x) = null; }	//!< Safe D3D-style release
	#define SAFE_DESTRUCT(x)	if (x) { (x)->SelfDestruct();	(x) = null; }	//!< Safe ICE-style release

#ifdef __ICEERROR_H__
	#define CHECKALLOC(x)		if(!x) return SetIceError("Out of memory.", EC_OUTOFMEMORY);	//!< Standard alloc checking. HANDLE WITH CARE.
#else
	#define CHECKALLOC(x)		if(!x) return false;
#endif

#endif // __ICEMEMORYMACROS_H__

#include "stdafx.h"
#pragma hdrstop

#include "cpuid.h"

/***
*
* int _cpuid (_p_info *pinfo)
* 
* Entry:
*
*   pinfo: pointer to _p_info, NULL is not allowed!
*
* Exit:
*
*   Returns int with capablity bit set.
*
****************************************************/
#ifdef _EDITOR
int _cpuid ( _processor_info *pinfo )
{
    ZeroMemory(pinfo, sizeof(_processor_info));

    pinfo->feature = _CPU_FEATURE_MMX | _CPU_FEATURE_SSE;
    return pinfo->feature;
}
#else

int _cpuid ( _processor_info *pinfo )
{__asm {

	// set pointers
	mov			edi , DWORD PTR [pinfo]

	// zero result
	xor			esi , esi
	mov			BYTE PTR  [edi][_processor_info::model_name][0] , 0
	mov			BYTE PTR  [edi][_processor_info::v_name][0] , 0

	// test for CPUID presence
	pushfd
	pop			eax
	mov			ebx , eax
	xor			eax , 00200000h
	push		eax
	popfd
	pushfd
	pop			eax
	cmp			eax , ebx
	jz			NO_CPUID

	// function 00h - query standard features
	xor			eax , eax
	cpuid
	
	mov			DWORD PTR [edi][_processor_info::v_name][0]  , ebx
	mov			DWORD PTR [edi][_processor_info::v_name][4]  , edx
	mov			DWORD PTR [edi][_processor_info::v_name][8]  , ecx
	mov			BYTE PTR  [edi][_processor_info::v_name][12] , 0

	// check for greater function presence
	test		eax , eax
	jz			CHECK_EXT

	// function 01h - feature sets
	mov			eax , 01h
	cpuid

	// stepping ID
	mov			ebx , eax
	and			ebx , 0fh
	mov			BYTE PTR [edi][_processor_info::stepping] , bl

	// Model
	mov			ebx , eax
	shr			ebx , 04h
	and			ebx , 0fh
	mov			BYTE PTR [edi][_processor_info::model] , bl

	// Family
	mov			ebx , eax
	shr			ebx , 08h
	and			ebx , 0fh
	mov			BYTE PTR [edi][_processor_info::family] , bl

	// Raw features
	// TODO: check against vendor

	// Against SSE3
	xor			ebx , ebx
	mov			eax , _CPU_FEATURE_SSE3
	test		ecx , 01h
	cmovnz		ebx , eax
	or			esi , ebx

	// Against MONITOR/MWAIT
	xor			ebx , ebx
	mov			eax , _CPU_FEATURE_MWAIT
	test		ecx , 08h
	cmovnz		ebx , eax
	or			esi , ebx

	// Against SSSE3
	xor			ebx , ebx
	mov			eax , _CPU_FEATURE_SSSE3
	test		ecx , 0200h
	cmovnz		ebx , eax
	or			esi , ebx

	// Against SSE4.1
	xor			ebx , ebx
	mov			eax , _CPU_FEATURE_SSE4_1
	test		ecx , 080000h
	cmovnz		ebx , eax
	or			esi , ebx

	// Against SSE4.2
	xor			ebx , ebx
	mov			eax , _CPU_FEATURE_SSE4_2
	test		ecx , 0100000h
	cmovnz		ebx , eax
	or			esi , ebx

	// Against MMX
	xor			ebx , ebx
	mov			eax , _CPU_FEATURE_MMX
	test		edx , 0800000h
	cmovnz		ebx , eax
	or			esi , ebx

	// Against SSE
	xor			ebx , ebx
	mov			eax , _CPU_FEATURE_SSE
	test		edx , 02000000h
	cmovnz		ebx , eax
	or			esi , ebx

	// Against SSE2
	xor			ebx , ebx
	mov			eax , _CPU_FEATURE_SSE2
	test		edx , 04000000h
	cmovnz		ebx , eax
	or			esi , ebx

CHECK_EXT:
	// test for extended functions
	mov			eax , 80000000h
	cpuid
	cmp			eax , 80000004h
	jb			NO_CPUID

	// first 16 bytes
	mov			eax , 80000002h
	cpuid

	mov			DWORD PTR [edi][_processor_info::model_name][0]  , eax
	mov			DWORD PTR [edi][_processor_info::model_name][4]  , ebx
	mov			DWORD PTR [edi][_processor_info::model_name][8]  , ecx
	mov			DWORD PTR [edi][_processor_info::model_name][12] , edx

	// second 16 bytes
	mov			eax , 80000003h
	cpuid

	mov			DWORD PTR [edi][_processor_info::model_name][16] , eax
	mov			DWORD PTR [edi][_processor_info::model_name][20] , ebx
	mov			DWORD PTR [edi][_processor_info::model_name][24] , ecx
	mov			DWORD PTR [edi][_processor_info::model_name][28] , edx

	// third 16 bytes
	mov			eax , 80000004h
	cpuid

	mov			DWORD PTR [edi][_processor_info::model_name][32] , eax
	mov			DWORD PTR [edi][_processor_info::model_name][36] , ebx
	mov			DWORD PTR [edi][_processor_info::model_name][40] , ecx
	mov			DWORD PTR [edi][_processor_info::model_name][44] , edx

	// trailing zero
	mov			BYTE PTR  [edi][_processor_info::model_name][48] , 0	

	// trimming initials
	mov			ax , 020h

	// trailing spaces
	xor			ebx , ebx

TS_FIND_LOOP:
	cmp			BYTE PTR  [edi][ebx][_processor_info::model_name] , ah
	jz			TS_FIND_EXIT
	inc			ebx
	jmp short	TS_FIND_LOOP
TS_FIND_EXIT:

	test		ebx , ebx
	jz			TS_MOVE_EXIT

TS_MOVE_LOOP:
	dec			ebx
	cmp			BYTE PTR  [edi][ebx][_processor_info::model_name] , al
	jnz			TS_MOVE_EXIT
	mov			BYTE PTR  [edi][ebx][_processor_info::model_name] , ah
	jmp short	TS_MOVE_LOOP

TS_MOVE_EXIT:

	// heading spaces
	xor			ebx , ebx

HS_FIND_LOOP:
	cmp			BYTE PTR  [edi][ebx][_processor_info::model_name] , al
	jnz			HS_FIND_EXIT
	inc			ebx
	jmp short	HS_FIND_LOOP
HS_FIND_EXIT:

	test		ebx , ebx
	jz			HS_MOVE_EXIT

	xor			edx , edx

HS_MOVE_LOOP:
	mov			cl , BYTE PTR  [edi][ebx][_processor_info::model_name]
	mov			BYTE PTR  [edi][edx][_processor_info::model_name] , cl
	inc			ebx
	inc			edx
	test		cl , cl
	jnz			HS_MOVE_LOOP

HS_MOVE_EXIT:	

	// many spaces
	xor			ebx , ebx

MS_FIND_LOOP:
	// 1st character
	mov			cl , BYTE PTR  [edi][ebx][_processor_info::model_name]
	test		cl , cl
	jz			MS_FIND_EXIT
	cmp			cl , al
	jnz			MS_FIND_NEXT
	// 2nd character
	mov			edx , ebx
	inc			ebx
	mov			cl , BYTE PTR  [edi][ebx][_processor_info::model_name]
	test		cl , cl
	jz			MS_FIND_EXIT
	cmp			cl , al
	jnz			MS_FIND_NEXT
	// move
	jmp short	HS_MOVE_LOOP

MS_FIND_NEXT:
	inc			ebx
	jmp short	MS_FIND_LOOP
MS_FIND_EXIT:

NO_CPUID:
	
	mov		DWORD PTR [edi][_processor_info::feature] , esi
}

	return pinfo->feature;
}
#endif

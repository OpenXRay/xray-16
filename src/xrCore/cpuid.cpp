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

#undef _CPUID_DEBUG

int _cpuid ( _processor_info *pinfo )
{	
	unsigned int lpid_width , mlpp;
	#ifdef _CPUID_DEBUG
		unsigned int mlpc , mcc;
	#endif // _CPUID_DEBUG
__asm {

	// set pointers
	mov			edi , DWORD PTR [pinfo]

	// zero structure
	xor			eax, eax
	mov			ecx, TYPE _processor_info
	mov			esi, edi
	add			esi, ecx
	neg			ecx
NZ:
	mov			BYTE PTR [esi][ecx], al
	inc			ecx
	jnz			NZ

	// zero result mask
	xor			esi , esi

	// zero bit width
	mov			DWORD PTR [lpid_width] , esi

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

	// Check for Intel signature
	cmp			ecx , 0x6C65746E				; "ntel"
	jnz			NO_HTT

	// Check for HTT bit
	mov			eax , 01h
	cpuid
	test		edx , 010000000h
	jz			NO_HTT

	// Max logical processors addressed in this package
	shr			ebx , 16
	and			ebx , 0FFh
	mov			DWORD PTR [mlpp] , ebx

	// How many cores we have?

	// Check for 04h leaf
	xor			eax , eax
	cpuid
	cmp			eax , 04h
	jb			ONE_CORE						; undocumented old P4 ?

	// Max addressable cores we have
	xor			ecx , ecx
	mov			eax , 04h
	cpuid

	shr			eax , 26
	and			eax , 03Fh
	jmp short	CALC_WIDTH

ONE_CORE:
	xor			eax , eax

CALC_WIDTH:
	inc			eax

#ifdef _CPUID_DEBUG
	mov			DWORD PTR [mcc] , eax
#endif // _CPUID_DEBUG

	// Addressable logical processors per core
	mov			ebx , eax
	xor			edx , edx
	mov			eax , DWORD PTR [mlpp]
	div			ebx

#ifdef _CPUID_DEBUG
	mov			DWORD PTR [mlpc] , eax
#endif // _CPUID_DEBUG

	cmp			eax , 01h
	jbe			NO_HTT

	// Calculate required bit width to address logical procesors
	xor			ecx , ecx
	mov			edx , ecx
	dec			eax
	bsr			cx , ax
	jz			BW_READY
	inc			cx
	mov			edx , ecx
BW_READY:
	mov			DWORD PTR [lpid_width] , edx

	// We have some sort of HT
	or			esi , _CPU_FEATURE_HTT

NO_HTT:
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

	// Standard features

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
#ifdef _CPUID_DEBUG
	printf("mlpp = %u\n" , mlpp );
	printf("mcc = %u\n" , mcc );
	printf("mlpc = %u\n" , mlpc );
	printf("\nlogical_id bit width = %u\n\n" , lpid_width );
#endif // _CPUID_DEBUG

	// Calculate available processors
	DWORD pa_mask_save, sa_mask_stub, pa_mask_test, proc_count = 0;

	GetProcessAffinityMask( GetCurrentProcess() , &pa_mask_save , &sa_mask_stub );

	pa_mask_test = pa_mask_save;
	while ( pa_mask_test ) {
		if ( pa_mask_test & 0x01 )
			++proc_count;
		pa_mask_test >>= 1;
	}

	// All logical processors
	pinfo->n_threads = proc_count;

	// easy case, HT is not possible at all
	if ( lpid_width == 0 ) {
		pinfo->affinity_mask = pa_mask_save;
		pinfo->n_cores = proc_count;
		return pinfo->feature;
	}
	
	// create APIC ID list
	DWORD dwAPIC_IDS[256], dwNums[256], n_cpu = 0 , n_avail = 0 , dwAPIC_ID , ta_mask;

	pa_mask_test = pa_mask_save;
	while ( pa_mask_test ) {
		if ( pa_mask_test & 0x01 ) {
			// Switch thread to specific CPU
			ta_mask = ( 1 << n_cpu );
			SetThreadAffinityMask( GetCurrentThread() , ta_mask );
			Sleep( 100 );
			// get APIC ID
			__asm {
				mov		eax , 01h
				cpuid
				shr		ebx , 24
				and		ebx , 0FFh
				mov		DWORD PTR [dwAPIC_ID] , ebx
			}

			#ifdef _CPUID_DEBUG
				char mask[255];
				_itoa_s( dwAPIC_ID , mask , 2 );
				printf("APID_ID #%2.2u = 0x%2.2X (%08.8sb)\n" , n_avail , dwAPIC_ID , mask );
			#endif // _CPUID_DEBUG

			// search for the APIC_ID with the same base
			BOOL bFound = FALSE;
			for ( DWORD i = 0 ; i < n_avail ; ++i )
				if ( ( dwAPIC_ID >> lpid_width ) == ( dwAPIC_IDS[i] >> lpid_width ) ) {
					bFound = TRUE;
					break;
				}
			if ( ! bFound ) {
				// add unique core
				dwNums[n_avail] = n_cpu;
				dwAPIC_IDS[n_avail] = dwAPIC_ID;
				++n_avail;
			}
		}
		// pick the next logical processor
		++n_cpu;
		pa_mask_test >>= 1;
	}

	// restore original saved affinity mask
	SetThreadAffinityMask( GetCurrentThread() , pa_mask_save );
	Sleep( 100 );

	// Create recommended mask
	DWORD ta_rec_mask = 0;
	for ( DWORD i = 0 ; i < n_avail ; ++i )
		ta_rec_mask |= ( 1 << dwNums[i] );

	pinfo->affinity_mask = ta_rec_mask;
	pinfo->n_cores = n_avail;

	return pinfo->feature;
}
#endif

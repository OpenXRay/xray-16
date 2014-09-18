#ifndef __XR_MATH_H__
#define __XR_MATH_H__

#include "cpuid.h"

namespace FPU {
	XRCORE_API void	 m24	(void);
	XRCORE_API void	 m24r	(void);	
	XRCORE_API void	 m53	(void);	
	XRCORE_API void	 m53r	(void);	
	XRCORE_API void	 m64	(void);	
	XRCORE_API void	 m64r	(void);	
};
namespace CPU {
	XRCORE_API extern u64				clk_per_second		;
	XRCORE_API extern u64				clk_per_milisec		;
	XRCORE_API extern u64				clk_per_microsec	;
	XRCORE_API extern u64				clk_overhead		;
	XRCORE_API extern float				clk_to_seconds		;
	XRCORE_API extern float				clk_to_milisec		;
	XRCORE_API extern float				clk_to_microsec		;
	
	XRCORE_API extern u64				qpc_freq			;
	XRCORE_API extern u64				qpc_overhead		;
	XRCORE_API extern u32				qpc_counter			;

	XRCORE_API extern	_processor_info	ID					;
	XRCORE_API extern	u64				QPC	()				;

#ifdef M_VISUAL
	#ifndef _M_AMD64
		#pragma warning(disable:4035)
		IC u64	GetCLK(void)	{
			_asm    _emit 0x0F;
			_asm    _emit 0x31;
		}
		#pragma warning(default:4035)
	#else
		IC u64	GetCLK(void)	{
			return __rdtsc();
		}
	#endif
#endif

#ifdef M_BORLAND
	XRCORE_API u64 __fastcall	GetCLK				(void);
#endif
};

extern XRCORE_API	void	_initialize_cpu			();
extern XRCORE_API	void	_initialize_cpu_thread	();

// threading
typedef				void	thread_t				( void * );
extern XRCORE_API	void	thread_name				( const char* name);
extern XRCORE_API	void	thread_spawn			(
	thread_t*	entry,
	const char*	name,
	unsigned	stack,
	void*		arglist 
	);

#endif //__XR_MATH_H__

#include "stdafx.h"
#pragma hdrstop

#include <process.h>

// mmsystem.h
#define MMNOSOUND
#define MMNOMIDI
#define MMNOAUX
#define MMNOMIXER
#define MMNOJOY
#include <mmsystem.h>

// Initialized on startup
XRCORE_API	Fmatrix			Fidentity;
XRCORE_API	Dmatrix			Didentity;
XRCORE_API	CRandom			Random;

#ifdef _M_AMD64
u16			getFPUsw()		{ return 0;	}

namespace	FPU 
{
	XRCORE_API void 	m24		(void)	{
		_control87	( _PC_24,   MCW_PC );
		_control87	( _RC_CHOP, MCW_RC );
	}
	XRCORE_API void 	m24r	(void)	{
		_control87	( _PC_24,   MCW_PC );
		_control87	( _RC_NEAR, MCW_RC );
	}
	XRCORE_API void 	m53		(void)	{
		_control87	( _PC_53,   MCW_PC );
		_control87	( _RC_CHOP, MCW_RC );
	}
	XRCORE_API void 	m53r	(void)	{
		_control87	( _PC_53,   MCW_PC );
		_control87	( _RC_NEAR, MCW_RC );
	}
	XRCORE_API void 	m64		(void)	{
		_control87	( _PC_64,   MCW_PC );
		_control87	( _RC_CHOP, MCW_RC );
	}
	XRCORE_API void 	m64r	(void)	{
		_control87	( _PC_64,   MCW_PC );
		_control87	( _RC_NEAR, MCW_RC );
	}

	void		initialize		()				{}
};
#else
u16 getFPUsw() 
{
	u16		SW;
	__asm	fstcw SW;
	return	SW;
}

namespace FPU 
{
	u16			_24	=0;
	u16			_24r=0;
	u16			_53	=0;
	u16			_53r=0;
	u16			_64	=0;
	u16			_64r=0;

	XRCORE_API void 	m24		()	{
		u16		p	= _24;
		__asm fldcw p;	
	}
	XRCORE_API void 	m24r	()	{
		u16		p	= _24r;
		__asm fldcw p;  
	}
	XRCORE_API void 	m53		()	{
		u16		p	= _53;
		__asm fldcw p;	
	}
	XRCORE_API void 	m53r	()	{
		u16		p	= _53r;
		__asm fldcw p;	
	}
	XRCORE_API void 	m64		()	{ 
		u16		p	= _64;
		__asm fldcw p;	
	}
	XRCORE_API void 	m64r	()	{
		u16		p	= _64r;
		__asm fldcw p;  
	}

	void		initialize		()
	{
		_clear87	();

		_control87	( _PC_24,   MCW_PC );
		_control87	( _RC_CHOP, MCW_RC );
		_24			= getFPUsw();	// 24, chop
		_control87	( _RC_NEAR, MCW_RC );
		_24r		= getFPUsw();	// 24, rounding

		_control87	( _PC_53,   MCW_PC );
		_control87	( _RC_CHOP, MCW_RC );
		_53			= getFPUsw();	// 53, chop
		_control87	( _RC_NEAR, MCW_RC );
		_53r		= getFPUsw();	// 53, rounding

		_control87	( _PC_64,   MCW_PC );
		_control87	( _RC_CHOP, MCW_RC );
		_64			= getFPUsw();	// 64, chop
		_control87	( _RC_NEAR, MCW_RC );
		_64r		= getFPUsw();	// 64, rounding

#ifndef XRCORE_STATIC

		m24r		();

#endif	//XRCORE_STATIC

		::Random.seed	( u32(CPU::GetCLK()%(1i64<<32i64)) );
	}
};
#endif

namespace CPU 
{
	XRCORE_API u64				clk_per_second	;
	XRCORE_API u64				clk_per_milisec	;
	XRCORE_API u64				clk_per_microsec;
	XRCORE_API u64				clk_overhead	;
	XRCORE_API float			clk_to_seconds	;
	XRCORE_API float			clk_to_milisec	;
	XRCORE_API float			clk_to_microsec	;
	XRCORE_API u64				qpc_freq		= 0	;
	XRCORE_API u64				qpc_overhead	= 0	;
	XRCORE_API u32				qpc_counter		= 0	;
	
	XRCORE_API _processor_info	ID;

	XRCORE_API u64				QPC	()			{
		u64		_dest	;
		QueryPerformanceCounter			((PLARGE_INTEGER)&_dest);
		qpc_counter	++	;
		return	_dest	;
	}

#ifdef M_BORLAND
	u64	__fastcall GetCLK		(void)
	{
		_asm    db 0x0F;
		_asm    db 0x31;
	}
#endif

	void Detect	()
	{
		// General CPU identification
		if (!_cpuid	(&ID))	
		{
			// Core.Fatal		("Fatal error: can't detect CPU/FPU.");
			abort				();
		}

		// Number of processors
		SYSTEM_INFO siSysInfo;
		GetSystemInfo( &siSysInfo ); 
		ID.n_threads = siSysInfo.dwNumberOfProcessors;


		// Timers & frequency
		u64			start,end;
		u32			dwStart,dwTest;

		SetPriorityClass		(GetCurrentProcess(),REALTIME_PRIORITY_CLASS);

		// Detect Freq
		dwTest	= timeGetTime();
		do { dwStart = timeGetTime(); } while (dwTest==dwStart);
		start	= GetCLK();
		while (timeGetTime()-dwStart<1000) ;
		end		= GetCLK();
		clk_per_second = end-start;

		// Detect RDTSC Overhead
		clk_overhead	= 0;
		u64 dummy		= 0;
		for (int i=0; i<256; i++)	{
			start			=	GetCLK();
			clk_overhead	+=	GetCLK()-start-dummy;
		}
		clk_overhead		/=	256;

		// Detect QPC Overhead
		QueryPerformanceFrequency	((PLARGE_INTEGER)&qpc_freq)	;
		qpc_overhead	= 0;
		for (i=0; i<256; i++)	{
			start			=	QPC();
			qpc_overhead	+=	QPC()-start-dummy;
		}
		qpc_overhead		/=	256;

		SetPriorityClass	(GetCurrentProcess(),NORMAL_PRIORITY_CLASS);

		clk_per_second	-=	clk_overhead;
		clk_per_milisec	=	clk_per_second/1000;
		clk_per_microsec	=	clk_per_milisec/1000;

		_control87	( _PC_64,   MCW_PC );
//		_control87	( _RC_CHOP, MCW_RC );
		double a,b;
		a = 1;		b = double(clk_per_second);
		clk_to_seconds = float(double(a/b));
		a = 1000;	b = double(clk_per_second);
		clk_to_milisec = float(double(a/b));
		a = 1000000;b = double(clk_per_second);
		clk_to_microsec = float(double(a/b));
	}
};

bool g_initialize_cpu_called = false;

//------------------------------------------------------------------------------------
void _initialize_cpu	(void) 
{
	Msg("* Detected CPU: %s [%s], F%d/M%d/S%d, %.2f mhz, %d-clk 'rdtsc'",
		CPU::ID.model_name,CPU::ID.v_name,
		CPU::ID.family,CPU::ID.model,CPU::ID.stepping,
		float(CPU::clk_per_second/u64(1000000)),
		u32(CPU::clk_overhead)
		);

//	DUMP_PHASE;

	if (strstr(Core.Params,"-x86"))		{
		CPU::ID.feature	&= ~_CPU_FEATURE_MMX	;
		CPU::ID.feature	&= ~_CPU_FEATURE_3DNOW	;
		CPU::ID.feature	&= ~_CPU_FEATURE_SSE	;
		CPU::ID.feature	&= ~_CPU_FEATURE_SSE2	;
		CPU::ID.feature	&= ~_CPU_FEATURE_SSE3	;
		CPU::ID.feature	&= ~_CPU_FEATURE_SSSE3	;
		CPU::ID.feature	&= ~_CPU_FEATURE_SSE4_1	;
		CPU::ID.feature	&= ~_CPU_FEATURE_SSE4_2	;
	};

	string256	features;	xr_strcpy(features,sizeof(features),"RDTSC");
    if (CPU::ID.feature&_CPU_FEATURE_MMX)	xr_strcat(features,", MMX");
    if (CPU::ID.feature&_CPU_FEATURE_3DNOW)	xr_strcat(features,", 3DNow!");
    if (CPU::ID.feature&_CPU_FEATURE_SSE)	xr_strcat(features,", SSE");
    if (CPU::ID.feature&_CPU_FEATURE_SSE2)	xr_strcat(features,", SSE2");
    if (CPU::ID.feature&_CPU_FEATURE_SSE3)	xr_strcat(features,", SSE3");
    if (CPU::ID.feature&_CPU_FEATURE_MWAIT)	xr_strcat(features,", MONITOR/MWAIT");
    if (CPU::ID.feature&_CPU_FEATURE_SSSE3)	xr_strcat(features,", SSSE3");
    if (CPU::ID.feature&_CPU_FEATURE_SSE4_1)xr_strcat(features,", SSE4.1");
    if (CPU::ID.feature&_CPU_FEATURE_SSE4_2)xr_strcat(features,", SSE4.2");
	Msg("* CPU features: %s" , features );

	Msg("* CPU threads: %d\n" , CPU::ID.n_threads );

	Fidentity.identity		();	// Identity matrix
	Didentity.identity		();	// Identity matrix
	pvInitializeStatics		();	// Lookup table for compressed normals
	FPU::initialize			();
	_initialize_cpu_thread	();

	g_initialize_cpu_called = true;
}

#ifdef M_BORLAND
void _initialize_cpu_thread	()
{
}
#else
// per-thread initialization
#include <xmmintrin.h>
#define _MM_DENORMALS_ZERO_MASK 0x0040
#define _MM_DENORMALS_ZERO_ON 0x0040
#define _MM_FLUSH_ZERO_MASK 0x8000
#define _MM_FLUSH_ZERO_ON 0x8000
#define _MM_SET_FLUSH_ZERO_MODE(mode) _mm_setcsr((_mm_getcsr() & ~_MM_FLUSH_ZERO_MASK) | (mode))
#define _MM_SET_DENORMALS_ZERO_MODE(mode) _mm_setcsr((_mm_getcsr() & ~_MM_DENORMALS_ZERO_MASK) | (mode))
static	BOOL	_denormals_are_zero_supported	= TRUE;
extern void __cdecl _terminate		();
void debug_on_thread_spawn	();

void _initialize_cpu_thread	()
{
	debug_on_thread_spawn	();
#ifndef XRCORE_STATIC
	// fpu & sse 
	FPU::m24r	();
#endif  // XRCORE_STATIC
	if (CPU::ID.feature&_CPU_FEATURE_SSE)	{
		//_mm_setcsr ( _mm_getcsr() | (_MM_FLUSH_ZERO_ON+_MM_DENORMALS_ZERO_ON) );
		_MM_SET_FLUSH_ZERO_MODE			(_MM_FLUSH_ZERO_ON);
		if (_denormals_are_zero_supported)	{
			__try	{
				_MM_SET_DENORMALS_ZERO_MODE	(_MM_DENORMALS_ZERO_ON);
			} __except(EXCEPTION_EXECUTE_HANDLER) {
				_denormals_are_zero_supported	= FALSE;
			}
		}
	}
}
#endif
// threading API 
#pragma pack(push,8)
struct THREAD_NAME	{
	DWORD	dwType;
	LPCSTR	szName;
	DWORD	dwThreadID;
	DWORD	dwFlags;
};
void	thread_name	(const char* name)
{
	THREAD_NAME		tn;
	tn.dwType		= 0x1000;
	tn.szName		= name;
	tn.dwThreadID	= DWORD(-1);
	tn.dwFlags		= 0;
	__try
	{
		RaiseException(0x406D1388,0,sizeof(tn)/sizeof(DWORD),(DWORD*)&tn);
	}
	__except(EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}
#pragma pack(pop)

struct	THREAD_STARTUP
{
	thread_t*	entry	;
	char*		name	;
	void*		args	;
};
void	__cdecl			thread_entry	(void*	_params )	{
	// initialize
	THREAD_STARTUP*		startup	= (THREAD_STARTUP*)_params	;
	thread_name			(startup->name);
	thread_t*			entry	= startup->entry;
	void*				arglist	= startup->args;
	xr_delete			(startup);
	_initialize_cpu_thread		();

	// call
	entry				(arglist);
}

void	thread_spawn	(thread_t*	entry, const char*	name, unsigned	stack, void* arglist )
{
	Debug._initialize	(false);

	THREAD_STARTUP*		startup	= xr_new<THREAD_STARTUP>	();
	startup->entry		= entry;
	startup->name		= (char*)name;
	startup->args		= arglist;
	_beginthread		(thread_entry,stack,startup);
}

void spline1	( float t, Fvector *p, Fvector *ret )
{
	float     t2  = t * t;
	float     t3  = t2 * t;
	float     m[4];

	ret->x=0.0f;
	ret->y=0.0f;
	ret->z=0.0f;
	m[0] = ( 0.5f * ( (-1.0f * t3) + ( 2.0f * t2) + (-1.0f * t) ) );
	m[1] = ( 0.5f * ( ( 3.0f * t3) + (-5.0f * t2) + ( 0.0f * t) + 2.0f ) );
	m[2] = ( 0.5f * ( (-3.0f * t3) + ( 4.0f * t2) + ( 1.0f * t) ) );
	m[3] = ( 0.5f * ( ( 1.0f * t3) + (-1.0f * t2) + ( 0.0f * t) ) );

	for( int i=0; i<4; i++ )
	{
		ret->x += p[i].x * m[i];
		ret->y += p[i].y * m[i];
		ret->z += p[i].z * m[i];
	}
}

void spline2( float t, Fvector *p, Fvector *ret )
{
	float	s= 1.0f - t;
	float   t2 = t * t;
	float   t3 = t2 * t;
	float   m[4];

	m[0] = s*s*s;
	m[1] = 3.0f*t3 - 6.0f*t2 + 4.0f;
	m[2] = -3.0f*t3 + 3.0f*t2 + 3.0f*t +1;
	m[3] = t3;

	ret->x = (p[0].x*m[0]+p[1].x*m[1]+p[2].x*m[2]+p[3].x*m[3])/6.0f;
	ret->y = (p[0].y*m[0]+p[1].y*m[1]+p[2].y*m[2]+p[3].y*m[3])/6.0f;
	ret->z = (p[0].z*m[0]+p[1].z*m[1]+p[2].z*m[2]+p[3].z*m[3])/6.0f;
}

#define beta1 1.0f
#define beta2 0.8f

void spline3( float t, Fvector *p, Fvector *ret )
{
	float	s= 1.0f - t;
	float   t2 = t * t;
	float   t3 = t2 * t;
	float	b12=beta1*beta2;
	float	b13=b12*beta1;
	float	delta=2.0f-b13+4.0f*b12+4.0f*beta1+beta2+2.0f;
	float	d=1.0f/delta;
	float	b0=2.0f*b13*d*s*s*s;
	float	b3=2.0f*t3*d;
	float	b1=d*(2*b13*t*(t2-3*t+3)+2*b12*(t3-3*t2+2)+2*beta1*(t3-3*t+2)+beta2*(2*t3-3*t2+1));
	float	b2=d*(2*b12*t2*(-t+3)+2*beta1*t*(-t2+3)+beta2*t2*(-2*t+3)+2*(-t3+1));

	ret->x = p[0].x*b0+p[1].x*b1+p[2].x*b2+p[3].x*b3;
	ret->y = p[0].y*b0+p[1].y*b1+p[2].y*b2+p[3].y*b3;
	ret->z = p[0].z*b0+p[1].z*b1+p[2].z*b2+p[3].z*b3;
}

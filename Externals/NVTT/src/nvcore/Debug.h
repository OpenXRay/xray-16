// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_DEBUG_H
#define NV_CORE_DEBUG_H

#include <nvcore/nvcore.h>

#if defined(HAVE_STDARG_H)
#	include <stdarg.h>	// va_list
#endif

#define NV_ABORT_DEBUG		1
#define NV_ABORT_IGNORE		2
#define NV_ABORT_EXIT		3

#undef assert	// avoid conflicts with assert method.

#define nvNoAssert(exp) \
	do { \
		(void)sizeof(exp); \
	} while(0)

#if NV_NO_ASSERT

#	define nvAssert(exp) nvNoAssert(exp)
#	define nvCheck(exp) nvNoAssert(exp)
#	define nvDebugAssert(exp) nvNoAssert(exp)
#	define nvDebugCheck(exp) nvNoAssert(exp)
#	define nvDebugBreak() nvNoAssert(0)

#else // NV_NO_ASSERT

#   if NV_CC_MSVC
        // @@ Does this work in msvc-6 and earlier?
#       define nvDebugBreak()       __debugbreak()
//#       define nvDebugBreak()        __asm { int 3 }
#   elif NV_OS_ORBIS
#       define nvDebugBreak()       __debugbreak()
#   elif NV_CC_GNUC
#       define nvDebugBreak()       __builtin_trap()
#   else
#       error "No nvDebugBreak()!"
#   endif

/*
#	if NV_CC_MSVC
		// @@ Does this work in msvc-6 and earlier?
		// @@ Do I have to include <intrin.h> ?
#		define nvDebugBreak()		__debugbreak()
		// define nvDebugBreak()		__asm int 3
#	elif NV_CC_GNUC && NV_CPU_PPC && NV_OS_DARWIN
#		define nvDebugBreak()		__asm__ volatile ("trap");
#	elif NV_CC_GNUC && NV_CPU_X86 && NV_OS_DARWIN
#		define nvDebugBreak()		__asm__ volatile ("int3");
#	elif NV_CC_GNUC && NV_CPU_X86
#		define nvDebugBreak()		__asm__ ( "int %0" : :"I"(3) )
#	else
#		include <signal.h>
#		define nvDebugBreak()		raise(SIGTRAP); 
		// define nvDebugBreak() 		*((int *)(0)) = 0
#	endif
*/

#	define nvAssertMacro(exp) \
		do { \
			if(!(exp)) { \
				if( nvAbort(#exp, __FILE__, __LINE__, __FUNC__) == NV_ABORT_DEBUG ) { \
					nvDebugBreak(); \
				} \
			} \
		} while(false)

#	define nvAssert(exp)	nvAssertMacro(exp)
#	define nvCheck(exp)		nvAssertMacro(exp)

#	if defined(_DEBUG)
#		define nvDebugAssert(exp)	nvAssertMacro(exp)
#		define nvDebugCheck(exp)	nvAssertMacro(exp)
#	else // _DEBUG
#		define nvDebugAssert(exp)	nvNoAssert(exp)
#		define nvDebugCheck(exp)	nvNoAssert(exp)
#	endif // _DEBUG

#endif // NV_NO_ASSERT

// Use nvAssume for very simple expresions only: nvAssume(0), nvAssume(value == true), etc.
#if defined(_DEBUG)
#	if NV_CC_MSVC
#		define nvAssume(exp)	__assume(exp)
#	else
#		define nvAssume(exp)	nvCheck(exp)
#	endif
#else
#	define nvAssume(exp)	nvCheck(exp)
#endif


#define nvError(x)		nvAbort(x, __FILE__, __LINE__, __FUNC__)
#define nvWarning(x)	nvDebug("*** Warning %s/%d: %s\n", __FILE__, __LINE__, (x))


#if PI_CC_MSVC
// @@ I'm not sure it's a good idea to use the default static assert.
#	define nvStaticCheck(x) _STATIC_ASSERT(x)
#else
#	define nvStaticCheck(x) typedef char NV_DO_STRING_JOIN2(__static_assert_,__LINE__)[(x)]
// 	define nvStaticCheck(x) switch(0) { case 0: case x:; }
#endif

NVCORE_API int nvAbort(const char *exp, const char *file, int line, const char * func = 0);
NVCORE_API void NV_CDECL nvDebug( const char *msg, ... ) __attribute__((format (printf, 1, 2)));

namespace nv
{
	/** Message handler interface. */
	struct MessageHandler {
		virtual void log(const char * str, va_list arg) = 0;
		virtual ~MessageHandler() {}	
	};
	
	/** Assert handler interface. */
	struct AssertHandler {
		virtual int assert(const char *exp, const char *file, int line, const char *func = 0) = 0;
		virtual ~AssertHandler() {}	
	};


	namespace debug
	{
		NVCORE_API void dumpInfo();
	
		// These functions are not thread safe.
		NVCORE_API void setMessageHandler( MessageHandler * messageHandler );
		NVCORE_API void resetMessageHandler();
	
		NVCORE_API void setAssertHandler( AssertHandler * assertHanlder );
		NVCORE_API void resetAssertHandler();
	
		NVCORE_API void enableSigHandler();
		NVCORE_API void disableSigHandler();
	}

} // nv namespace

#endif	// NV_CORE_DEBUG_H

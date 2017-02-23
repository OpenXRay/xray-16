#ifndef __GSIASSERT_H__
#define __GSIASSERT_H__

#if defined(__LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  Assert for GameSpy SDKs
//
//  Usage:
//      1) #define _DEBUG to enable assert.  This should be set by compiler configuration.
//
//  Todo:
//      Allow user to specify IP to send debug output to (remote log for PS2)


// GS_ASSERT	Use this to trap any programming bugs, such as range checks, invalid parameters
// use at start of each function to check all parameters
// also check all assumptions, ex// assume module is init.
//

// GS_FAIL()	Use instead of GS_ASSERT(0) when reaching an illegal area of code 
//				ex// the default: in a case statement that should be completely handled.


/*
	***The reason for using a GameSpy assert or custom assert***
	
	 although assert is handled very gracefully on the windows platform, most consoles do very little of real use during assert.
	 Furthermore, the program counter is lost, along with the callstack sometimes.
	 By having a custom critical error function, an asm "break" can be set in it, or a debugger break point.  a call stack is
	 immediately available.  The error can be drawn onto the screen.  And the choice to ignore can also be given, in order
	 to continue stepping through code and further debug.
	
*/

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
extern  void  gsDebugAssert			(const char *format,const char *szError,const char *szFile, int line);
#ifndef _DEBUG
	// On non-debug builds, release builds, ignore all of this.
	// be carefull never to have function calls within one of these macros, as they will 
	// be ignored.
	// ex// BAD:  GS_ASSERT( i== FN())		// FN() will never be called, i will never be set in release builds

	#define GS_ASSERT(x)				{};		// ex// GS_ASSERT(		result == GS_OK )
	#define GS_ASSERT_STR(x, t)			{};		// ex// GS_ASSERT_STR(	result == GS_OK ,"GSFunction failed")
	#define GS_ASSERT_ALIGN_16(x)		{}; 
	#define GS_FAIL()			
	#define GS_FAIL_STR(x)			

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#else

	#define GS_ASSERT(x)				{ if(!(x))			{  gsDebugAssert("ASSERT on '" #x "' [%s] in %s line:%d\n",	"",					__FILE__,__LINE__); } };
	#define GS_ASSERT_STR(x,t)			{ if(!(x))			{  gsDebugAssert("ASSERT on '" #x "' [%s] in %s line:%d\n",	 t,					__FILE__,__LINE__); } };
	#define GS_ASSERT_ALIGN_16(x)		{ if(((U32)(x))%16) {  gsDebugAssert("ASSERT on '" #x "' [%s] in %s line:%d\n","16 byte misalign",	__FILE__,__LINE__); } };
	#define GS_FAIL()										{  gsDebugAssert("FAIL  [%s] ln %s line:%d\n",	"",					__FILE__,__LINE__);	};											
	#define GS_FAIL_STR(t)									{  gsDebugAssert("FAIL  [%s] ln %s line:%d\n",	t,					__FILE__,__LINE__);	};											
																				  


#endif // GSI_COMMON_DEBUG


// This is the default assert condition handler
typedef void (*gsDebugAssertCallback)	(const char *string);
	
//  Call this function to override the default assert handler  
//	New function should render message / log message based on string passed
//  calling this with NULL is restores the default setting.
void gsDebugAssertCallbackSet(gsDebugAssertCallback theCallback);


// This is like an assert, but test at compile, not run time.
// ex use STATIC_CHECK(DIM(array) == enumArrayCount)
#define GS_STATIC_CHECK(expr, msg)    { CompileTimeError<((expr) != 0)> ERROR_##msg; (void)ERROR_##msg; } 


#if defined(__LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif

#endif // __GSIDEBUG_H__

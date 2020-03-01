// This code is in the public domain -- castanyo@yahoo.es

#include <nvcore/Debug.h>
#include <nvcore/StrLib.h>

// Extern
#if NV_OS_WIN32 //&& NV_CC_MSVC
#	define WIN32_LEAN_AND_MEAN
#	define VC_EXTRALEAN
#	include <windows.h>
#	include <direct.h>
#	if NV_CC_MSVC
#		include <crtdbg.h>
#		if _MSC_VER < 1300
#			define DECLSPEC_DEPRECATED
			// VC6: change this path to your Platform SDK headers
#			include <dbghelp.h>	// must be XP version of file
//			include "M:\\dev7\\vs\\devtools\\common\\win32sdk\\include\\dbghelp.h"
#		else
			// VC7: ships with updated headers
#			include <dbghelp.h>
#		endif
#	endif
#endif

#if !NV_OS_WIN32 && defined(HAVE_SIGNAL_H)
#	include <signal.h>
#endif

#if NV_OS_LINUX && defined(HAVE_EXECINFO_H)
#	include <sys/types.h>
#	include <unistd.h> // getpid
#	include <execinfo.h> // backtrace
#	if NV_CC_GNUC // defined(HAVE_CXXABI_H)
#		include <cxxabi.h>
#	endif
#endif

#if NV_OS_DARWIN
#	include <unistd.h>	// getpid
#	include <sys/types.h>
#	include <sys/sysctl.h>	// sysctl
#	include <sys/ucontext.h>
#	undef HAVE_EXECINFO_H
#	if defined(HAVE_EXECINFO_H) // only after OSX 10.5
#		include <execinfo.h> // backtrace
#		if NV_CC_GNUC // defined(HAVE_CXXABI_H)
#			include <cxxabi.h>
#		endif
#	endif
#endif

#include <stdexcept> // std::runtime_error
#undef assert // defined on mingw

using namespace nv;

namespace 
{

	static MessageHandler * s_message_handler = NULL;
	static AssertHandler * s_assert_handler = NULL;
	
	static bool s_sig_handler_enabled = false;

#if NV_OS_WIN32 && NV_CC_MSVC

	// Old exception filter.
	static LPTOP_LEVEL_EXCEPTION_FILTER s_old_exception_filter = NULL;

#elif !NV_OS_WIN32 && defined(HAVE_SIGNAL_H)

	// Old signal handlers.
	struct sigaction s_old_sigsegv;
	struct sigaction s_old_sigtrap;
	struct sigaction s_old_sigfpe;
	struct sigaction s_old_sigbus;
	
#endif


#if NV_OS_WIN32 && NV_CC_MSVC

	// TODO write minidump
	
	static LONG WINAPI nvTopLevelFilter( struct _EXCEPTION_POINTERS * pExceptionInfo)
	{
		NV_UNUSED(pExceptionInfo);
	/*	BOOL (WINAPI * Dump) (HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION );
	
		AutoString dbghelp_path(512);
		getcwd(dbghelp_path, 512);
		dbghelp_path.Append("\\DbgHelp.dll");
		nvTranslatePath(dbghelp_path);
		
		PiLibrary DbgHelp_lib(dbghelp_path, true);
		
		if( !DbgHelp_lib.IsValid() ) {
			nvDebug("*** 'DbgHelp.dll' not found.\n");
			return EXCEPTION_CONTINUE_SEARCH;
		}
		
		if( !DbgHelp_lib.BindSymbol( (void **)&Dump, "MiniDumpWriteDump" ) ) {
			nvDebug("*** 'DbgHelp.dll' too old.\n");
			return EXCEPTION_CONTINUE_SEARCH;
		}
		
		// create the file
		HANDLE hFile = ::CreateFile( "nv.dmp", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if( hFile == INVALID_HANDLE_VALUE ) {
			nvDebug("*** Failed to create dump file.\n");
			return EXCEPTION_CONTINUE_SEARCH;
		}
		
		
		_MINIDUMP_EXCEPTION_INFORMATION ExInfo;
	
		ExInfo.ThreadId = ::GetCurrentThreadId();
		ExInfo.ExceptionPointers = pExceptionInfo;
		ExInfo.ClientPointers = NULL;
	
		// write the dump
		bool ok = Dump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL )!=0;
		::CloseHandle(hFile);
		
		if( !ok ) {
			nvDebug("*** Failed to save dump file.\n");
			return EXCEPTION_CONTINUE_SEARCH;
		}
		
		nvDebug("--- Dump file saved.\n");
		*/
		return EXCEPTION_CONTINUE_SEARCH;
	}

#elif !NV_OS_WIN32 && defined(HAVE_SIGNAL_H) // NV_OS_LINUX || NV_OS_DARWIN

#if defined(HAVE_EXECINFO_H) // NV_OS_LINUX

	static bool nvHasStackTrace() {
#if NV_OS_DARWIN
		return backtrace != NULL;
#else
		return true;
#endif
	}

	static void nvPrintStackTrace(void * trace[], int size, int start=0) {
		char ** string_array = backtrace_symbols(trace, size);
	
		nvDebug( "\nDumping stacktrace:\n" );
		for(int i = start; i < size-1; i++ ) {
#		if NV_CC_GNUC // defined(HAVE_CXXABI_H)
			char * begin = strchr(string_array[i], '(');
			char * end = strchr(string_array[i], '+');
			if( begin != 0 && begin < end ) {
				int stat;
				*end = '\0';
				*begin = '\0';
				char * module = string_array[i];
				char * name = abi::__cxa_demangle(begin+1, 0, 0, &stat);
				if( name == NULL || begin[1] != '_' || begin[2] != 'Z' ) {
					nvDebug( "  In: [%s] '%s'\n", module, begin+1 );
				}
				else {
					nvDebug( "  In: [%s] '%s'\n", module, name );
				}
				free(name);
			}
			else {
				nvDebug( "  In: '%s'\n", string_array[i] );
			}
#		else
			nvDebug( "  In: '%s'\n", string_array[i] );
#		endif
		}
		nvDebug("\n");
	
		free(string_array);
	}

#endif // defined(HAVE_EXECINFO_H)

	static void * callerAddress(void * secret)
	{
#	if NV_OS_DARWIN
#		if defined(_STRUCT_MCONTEXT)
#			if NV_CPU_PPC
				ucontext_t * ucp = (ucontext_t *)secret;
				return (void *) ucp->uc_mcontext->__ss.__srr0;
#			elif NV_CPU_X86
				ucontext_t * ucp = (ucontext_t *)secret;
				return (void *) ucp->uc_mcontext->__ss.__eip;
#			endif
#		else
#			if NV_CPU_PPC
				ucontext_t * ucp = (ucontext_t *)secret;
				return (void *) ucp->uc_mcontext->ss.srr0;
#			elif NV_CPU_X86
				ucontext_t * ucp = (ucontext_t *)secret;
				return (void *) ucp->uc_mcontext->ss.eip;
#			endif
#		endif
#	else
#		if NV_CPU_X86_64
			// #define REG_RIP REG_INDEX(rip) // seems to be 16
			ucontext_t * ucp = (ucontext_t *)secret;
			return (void *)ucp->uc_mcontext.gregs[REG_RIP];
#		elif NV_CPU_X86
			ucontext_t * ucp = (ucontext_t *)secret;
			return (void *)ucp->uc_mcontext.gregs[14/*REG_EIP*/];
#		elif NV_CPU_PPC
			ucontext_t * ucp = (ucontext_t *)secret;
			return (void *) ucp->uc_mcontext.regs->nip;
#		endif
#	endif
		
		// How to obtain the instruction pointers in different platforms, from mlton's source code.
		// http://mlton.org/
		// OpenBSD && NetBSD
		// ucp->sc_eip
		// FreeBSD:
		// ucp->uc_mcontext.mc_eip
		// HPUX:
		// ucp->uc_link
		// Solaris:
		// ucp->uc_mcontext.gregs[REG_PC]
		// Linux hppa:
		// uc->uc_mcontext.sc_iaoq[0] & ~0x3UL
		// Linux sparc:
		// ((struct sigcontext*) secret)->sigc_regs.tpc
		// Linux sparc64:
		// ((struct sigcontext*) secret)->si_regs.pc
	
		// potentially correct for other archs:
		// Linux alpha: ucp->m_context.sc_pc
		// Linux arm: ucp->m_context.ctx.arm_pc
		// Linux ia64: ucp->m_context.sc_ip & ~0x3UL
		// Linux mips: ucp->m_context.sc_pc
		// Linux s390: ucp->m_context.sregs->regs.psw.addr
	}
	
	static void nvSigHandler(int sig, siginfo_t *info, void *secret)
	{
		void * pnt = callerAddress(secret);
		
		// Do something useful with siginfo_t
		if (sig == SIGSEGV) {
			if (pnt != NULL) nvDebug("Got signal %d, faulty address is %p, from %p\n", sig, info->si_addr, pnt);
			else nvDebug("Got signal %d, faulty address is %p\n", sig, info->si_addr);
		}
		else if(sig == SIGTRAP) {
			nvDebug("Breakpoint hit.\n");
		}
		else {
			nvDebug("Got signal %d\n", sig);
		}
		
#	if defined(HAVE_EXECINFO_H)
		if (nvHasStackTrace()) // in case of weak linking
		{
			void * trace[64];
			int size = backtrace(trace, 64);
		
			if (pnt != NULL) {
				// Overwrite sigaction with caller's address.
				trace[1] = pnt;
			}
			
			nvPrintStackTrace(trace, size, 1);
		}
#	endif // defined(HAVE_EXECINFO_H)
		
		exit(0);
	}

#endif // defined(HAVE_SIGNAL_H)



#if NV_OS_WIN32 //&& NV_CC_MSVC
	
	/** Win32 asset handler. */
	struct Win32AssertHandler : public AssertHandler 
	{
		// Code from Daniel Vogel.
		static bool isDebuggerPresent()
		{
			bool result = false;
			
			HINSTANCE kern_lib = LoadLibraryExA( "kernel32.dll", NULL, 0 );
			if( kern_lib ) {
				FARPROC lIsDebuggerPresent = GetProcAddress( kern_lib, "IsDebuggerPresent" );
				if( lIsDebuggerPresent && lIsDebuggerPresent() ) {
					result = true;
				}
				
				FreeLibrary( kern_lib );
			}
			return result;
		}
		
		// Flush the message queue. This is necessary for the message box to show up.
		static void flushMessageQueue()
		{
			MSG msg;
			while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
				if( msg.message == WM_QUIT ) break;
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
	
		// Assert handler method.
		virtual int assert( const char * exp, const char * file, int line, const char * func/*=NULL*/ )
		{
			int ret = NV_ABORT_EXIT;
			
			StringBuilder error_string;
			if( func != NULL ) {
				error_string.format( "*** Assertion failed: %s\n    On file: %s\n    On function: %s\n    On line: %d\n ", exp, file, func, line );
				nvDebug( error_string );
			}
			else {
				error_string.format( "*** Assertion failed: %s\n    On file: %s\n    On line: %d\n ", exp, file, line );
				nvDebug( error_string );
			}
			
		#if _DEBUG
			
			if( isDebuggerPresent() ) {
				return NV_ABORT_DEBUG;
			}
			
			flushMessageQueue();
			int action = MessageBoxA(NULL, error_string, "Assertion failed", MB_ABORTRETRYIGNORE|MB_ICONERROR);
			switch( action ) {
				case IDRETRY:
					ret = NV_ABORT_DEBUG;
					break;
				case IDIGNORE:
					ret = NV_ABORT_IGNORE;
					break;
				case IDABORT:
				default:
					ret = NV_ABORT_EXIT;
					break;
			}
			/*if( _CrtDbgReport( _CRT_ASSERT, file, line, module, exp ) == 1 ) {
				return NV_ABORT_DEBUG;
			}*/
			
		#endif
			
			if( ret == NV_ABORT_EXIT ) {
				// Exit cleanly.
				throw std::runtime_error("Assertion failed");
			}
			
			return ret;			
		}
	};
	
#else
	
	/** Unix asset handler. */
	struct UnixAssertHandler : public AssertHandler
	{
		bool isDebuggerPresent()
		{
#		if NV_OS_DARWIN
			int mib[4];
			struct kinfo_proc info;
			size_t size;
			mib[0] = CTL_KERN;
			mib[1] = KERN_PROC;
			mib[2] = KERN_PROC_PID;
			mib[3] = getpid();
			size = sizeof(info);
			info.kp_proc.p_flag = 0;
			sysctl(mib,4,&info,&size,NULL,0);
			return ((info.kp_proc.p_flag & P_TRACED) == P_TRACED);
#		else
			// if ppid != sid, some process spawned our app, probably a debugger. 
			return getsid(getpid()) != getppid();
#		endif
		}
		
		// Assert handler method.
		virtual int assert(const char * exp, const char * file, int line, const char * func)
		{
			if( func != NULL ) {
				nvDebug( "*** Assertion failed: %s\n    On file: %s\n    On function: %s\n    On line: %d\n ", exp, file, func, line );
			}
			else {
				nvDebug( "*** Assertion failed: %s\n    On file: %s\n    On line: %d\n ", exp, file, line );
			}
			
#		if _DEBUG
			if( isDebuggerPresent() ) {
				return NV_ABORT_DEBUG;
			}
#		endif

#		if defined(HAVE_EXECINFO_H)
			if (nvHasStackTrace())
			{
				void * trace[64];
				int size = backtrace(trace, 64);
				nvPrintStackTrace(trace, size, 2);
			}
#		endif

			// Exit cleanly.
			throw std::runtime_error("Assertion failed");
		}
	};
	
#endif

} // namespace


/// Handle assertion through the asset handler.
int nvAbort(const char * exp, const char * file, int line, const char * func)
{
#if NV_OS_WIN32 //&& NV_CC_MSVC
	static Win32AssertHandler s_default_assert_handler;
#else
	static UnixAssertHandler s_default_assert_handler;
#endif
	
	if( s_assert_handler != NULL ) {
		return s_assert_handler->assert( exp, file, line, func );
	}
	else {
		return s_default_assert_handler.assert( exp, file, line, func );
	}
}


/// Shows a message through the message handler.
void NV_CDECL nvDebug(const char *msg, ...)
{
	va_list arg;
	va_start(arg,msg);
	if( s_message_handler != NULL ) {
		s_message_handler->log( msg, arg );
	}
	va_end(arg);
}


/// Dump debug info.
void debug::dumpInfo()
{
#if !NV_OS_WIN32 && defined(HAVE_SIGNAL_H) && defined(HAVE_EXECINFO_H)
	if (nvHasStackTrace())
	{
		void * trace[64];
		int size = backtrace(trace, 64);
		nvPrintStackTrace(trace, size, 1);
	}
#endif
}


/// Set the debug message handler.
void debug::setMessageHandler(MessageHandler * message_handler)
{
	s_message_handler = message_handler;
}

/// Reset the debug message handler.
void debug::resetMessageHandler()
{
	s_message_handler = NULL;
}

/// Set the assert handler.
void debug::setAssertHandler(AssertHandler * assert_handler)
{
	s_assert_handler = assert_handler;
}

/// Reset the assert handler.
void debug::resetAssertHandler()
{
	s_assert_handler = NULL;
}


/// Enable signal handler.
void debug::enableSigHandler()
{
	nvCheck(s_sig_handler_enabled != true);
	s_sig_handler_enabled = true;
	
#if NV_OS_WIN32 && NV_CC_MSVC
	
	s_old_exception_filter = ::SetUnhandledExceptionFilter( nvTopLevelFilter );
	
#elif !NV_OS_WIN32 && defined(HAVE_SIGNAL_H)
	
	// Install our signal handler
	struct sigaction sa;
	sa.sa_sigaction = nvSigHandler;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = SA_ONSTACK | SA_RESTART | SA_SIGINFO;

	sigaction(SIGSEGV, &sa, &s_old_sigsegv);
	sigaction(SIGTRAP, &sa, &s_old_sigtrap);
	sigaction(SIGFPE, &sa, &s_old_sigfpe);
	sigaction(SIGBUS, &sa, &s_old_sigbus);
	
#endif
}

/// Disable signal handler.
void debug::disableSigHandler()
{
	nvCheck(s_sig_handler_enabled == true);
	s_sig_handler_enabled = false;

#if NV_OS_WIN32 && NV_CC_MSVC

	::SetUnhandledExceptionFilter( s_old_exception_filter );
	s_old_exception_filter = NULL;

#elif !NV_OS_WIN32 && defined(HAVE_SIGNAL_H)
	
	sigaction(SIGSEGV, &s_old_sigsegv, NULL);
	sigaction(SIGTRAP, &s_old_sigtrap, NULL);
	sigaction(SIGFPE, &s_old_sigfpe, NULL);
	sigaction(SIGBUS, &s_old_sigbus, NULL);
	
#endif
}


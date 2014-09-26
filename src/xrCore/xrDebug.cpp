#include "stdafx.h"
#pragma hdrstop

#ifndef _EDITOR

#include "xrdebug.h"
#include "resource.h"
#include "dbghelp.h"
 
#include "dxerr.h"

#ifdef __BORLANDC__
	#include "d3d9.h"
	#include "d3dx9.h"
	#include "D3DX_Wrapper.h"
	#pragma comment		(lib,"EToolsB.lib")
	static BOOL			bException	= TRUE;
#else
	static BOOL			bException	= FALSE;
#endif

#ifdef _M_AMD64
#define DEBUG_INVOKE	DebugBreak	()
#else
#define DEBUG_INVOKE	__asm		{ int 3 }
#ifndef __BORLANDC__
	#pragma comment			(lib,"dxerr.lib")
#endif
#endif

XRCORE_API	xrDebug		Debug;

// Dialog support
static const char * dlgExpr		= NULL;
static const char * dlgFile		= NULL;
static char			dlgLine		[16];

static INT_PTR CALLBACK DialogProc	( HWND hw, UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg ){
	case WM_INITDIALOG:
		{
			if (dlgFile)
			{
				SetWindowText(GetDlgItem(hw,IDC_DESC),dlgExpr);
				SetWindowText(GetDlgItem(hw,IDC_FILE),dlgFile);
				SetWindowText(GetDlgItem(hw,IDC_LINE),dlgLine);
			} else {
				SetWindowText(GetDlgItem(hw,IDC_DESC),dlgExpr);
				SetWindowText(GetDlgItem(hw,IDC_FILE),"");
				SetWindowText(GetDlgItem(hw,IDC_LINE),"");
			}
		}
		break;
	case WM_DESTROY:
		break;
	case WM_CLOSE:
		EndDialog	(hw, IDC_STOP);
		break;
	case WM_COMMAND:
		if( LOWORD(wp)==IDC_STOP ) {
			EndDialog(hw, IDC_STOP);
		}
		if( LOWORD(wp)==IDC_DEBUG) {
			EndDialog(hw, IDC_DEBUG);
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void xrDebug::backend(const char* reason, const char* expression, const char *argument0, const char *argument1, const char* file, int line, const char *function, bool &ignore_always)
{
	static	xrCriticalSection	CS;

	CS.Enter			();

	// Log
	string1024			tmp;
	xr_sprintf				(tmp,"***STOP*** file '%s', line %d.\n***Reason***: %s\n %s",file,line,reason,expression);
	Msg					(tmp);
	FlushLog			();
	if (handler)		handler	();

	// Call the dialog
	dlgExpr				= reason;
    xr_sprintf             ()
	dlgFile				= file;
	xr_sprintf				(dlgLine,"%d",line);
	INT_PTR res			= -1;
#ifdef XRCORE_STATIC
	MessageBox			(NULL,tmp,"X-Ray error",MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
#else
	res	= DialogBox
		(
		GetModuleHandle(MODULE_NAME),
		MAKEINTRESOURCE(IDD_STOP),
		NULL,
		DialogProc 
		);
#endif
	switch (res) 
	{
	case -1:
	case IDC_STOP:
		if (bException)		TerminateProcess(GetCurrentProcess(),3);
		else				RaiseException	(0, 0, 0, NULL);
		break;
	case IDC_DEBUG:
 		DEBUG_INVOKE;
		break;
	}

	CS.Leave			();
}

LPCSTR xrDebug::error2string	(long code)
{
	LPCSTR				result	= 0;
	static	string1024	desc_storage;

#ifdef _M_AMD64
#else
	result				= DXGetErrorDescription	(code);
#endif
	if (0==result) 
	{
		FormatMessage	(FORMAT_MESSAGE_FROM_SYSTEM,0,code,0,desc_storage,sizeof(desc_storage)-1,0);
		result			= desc_storage;
	}
	return		result	;
}

void xrDebug::error		(long hr, const char* expr, const char *file, int line, const char *function, bool &ignore_always)
{
	backend		(error2string(hr),expr,0,0,file,line,function,ignore_always);
}

void xrDebug::error		(long hr, const char* expr, const char *e2, const char *file, int line, const char *function, bool &ignore_always)
{
	backend		(error2string(hr),expr,e2,0,file,line,function,ignore_always);
}

void xrDebug::fail		(const char *e1, const char *file, int line, const char *function, bool &ignore_always)
{
	backend		("assertion failed",e1,0,0,file,line,function,ignore_always);
}

void xrDebug::fail		(const char *e1, const char *e2, const char *file, int line, const char *function, bool &ignore_always)
{
	backend		(e1,e2,0,0,file,line,function,ignore_always);
}

void xrDebug::fail		(const char *e1, const char *e2, const char *e3, const char *file, int line, const char *function, bool &ignore_always)
{
	backend		(e1,e2,e3,0,file,line,function,ignore_always);
}

void xrDebug::fail		(const char *e1, const char *e2, const char *e3, const char *e4, const char *file, int line, const char *function, bool &ignore_always)
{
	backend		(e1,e2,e3,e4,file,line,function,ignore_always);
}

void __cdecl xrDebug::fatal(const char *file, int line, const char *function, const char* F,...)
{
	string1024	buffer;

	va_list		p;
	va_start	(p,F);
	vsprintf	(buffer,F,p);
	va_end		(p);

	bool		ignore_always = true;

	backend		("fatal error","<no expression>",buffer,0,file,line,function,ignore_always);
}
void xrDebug::do_exit	(const std::string &message)
{
	FlushLog			();
    MessageBox			(NULL,message.c_str(),"Error",MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
    TerminateProcess	(GetCurrentProcess(),1);
}

int __cdecl _out_of_memory	(size_t size)
{
	Debug.fatal				(DEBUG_INFO,"Out of memory. Memory request: %d K",size/1024);
	return					1;
}
void __cdecl _terminate		()
{
	FATAL					("Unexpected application termination");
}

// based on dbghelp.h
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
										 CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
										 CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
										 CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
										 );


LONG WINAPI UnhandledFilter	( struct _EXCEPTION_POINTERS *pExceptionInfo )
{
	LONG retval		= EXCEPTION_CONTINUE_SEARCH;
	bException		= TRUE;

	// firstly see if dbghelp.dll is around and has the function we need
	// look next to the EXE first, as the one in System32 might be old 
	// (e.g. Windows 2000)
	HMODULE hDll	= NULL;
	string_path		szDbgHelpPath;

	if (GetModuleFileName( NULL, szDbgHelpPath, _MAX_PATH ))
	{
		char *pSlash = strchr( szDbgHelpPath, '\\' );
		if (pSlash)
		{
			xr_strcpy	(pSlash+1, "DBGHELP.DLL" );
			hDll = ::LoadLibrary( szDbgHelpPath );
		}
	}

	if (hDll==NULL)
	{
		// load any version we can
		hDll = ::LoadLibrary( "DBGHELP.DLL" );
	}

	LPCTSTR szResult = NULL;

	if (hDll)
	{
		MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, "MiniDumpWriteDump" );
		if (pDump)
		{
			string_path	szDumpPath;
			string_path	szScratch;
			string64	t_stemp;

			// work out a good place for the dump file
			timestamp	(t_stemp);
			xr_strcpy		( szDumpPath, "logs\\"				);
			xr_strcat		( szDumpPath, Core.ApplicationName	);
			xr_strcat		( szDumpPath, "_"					);
			xr_strcat		( szDumpPath, Core.UserName			);
			xr_strcat		( szDumpPath, "_"					);
			xr_strcat		( szDumpPath, t_stemp				);
			xr_strcat		( szDumpPath, ".mdmp"				);

			// create the file
			HANDLE hFile = ::CreateFile( szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			if (INVALID_HANDLE_VALUE==hFile)	
			{
				// try to place into current directory
				MoveMemory	(szDumpPath,szDumpPath+5,strlen(szDumpPath));
				hFile		= ::CreateFile( szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			}
			if (hFile!=INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

				ExInfo.ThreadId				= ::GetCurrentThreadId();
				ExInfo.ExceptionPointers	= pExceptionInfo;
				ExInfo.ClientPointers		= NULL;

				// write the dump
				MINIDUMP_TYPE	dump_flags	= MINIDUMP_TYPE(MiniDumpNormal | MiniDumpFilterMemory | MiniDumpScanMemory );

				BOOL bOK = pDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, dump_flags, &ExInfo, NULL, NULL );
				if (bOK)
				{
					xr_sprintf( szScratch, "Saved dump file to '%s'", szDumpPath );
					szResult = szScratch;
					retval = EXCEPTION_EXECUTE_HANDLER;
				}
				else
				{
					xr_sprintf( szScratch, "Failed to save dump file to '%s' (error %d)", szDumpPath, GetLastError() );
					szResult = szScratch;
				}
				::CloseHandle(hFile);
			}
			else
			{
				xr_sprintf( szScratch, "Failed to create dump file '%s' (error %d)", szDumpPath, GetLastError() );
				szResult = szScratch;
			}
		}
		else
		{
			szResult = "DBGHELP.DLL too old";
		}
	}
	else
	{
		szResult = "DBGHELP.DLL not found";
	}

	string1024		reason;
	xr_sprintf			(reason,"*** Internal Error ***\n%s",szResult);
    bool ref		= false;
	Debug.backend	(reason,0,0,0,0,0,0,ref);

	return retval;
}

//////////////////////////////////////////////////////////////////////
#ifdef M_BORLAND
//	typedef void ( _RTLENTRY *___new_handler) ();
namespace std{
	extern new_handler _RTLENTRY _EXPFUNC set_new_handler( new_handler new_p );
};

//    typedef int	(__stdcall * _PNH)( size_t );
//    _CRTIMP int	__cdecl _set_new_mode( int );
//    _PNH	__cdecl set_new_handler( _PNH );
//	typedef void (new * new_handler)();
//	new_handler set_new_handler(new_handler my_handler);
	static void __cdecl def_new_handler() 
    {
		FATAL		("Out of memory.");
    }

    void	xrDebug::_initialize		(const bool &dedicated)
    {
//        std::set_new_mode 				(1);					// gen exception if can't allocate memory
        std::set_new_handler			(def_new_handler  );	// exception-handler for 'out of memory' condition
        ::SetUnhandledExceptionFilter	( UnhandledFilter );	// exception handler to all "unhandled" exceptions
    }
#else
    typedef int		(__cdecl * _PNH)( size_t );
    _CRTIMP int		__cdecl _set_new_mode( int );
    _CRTIMP _PNH	__cdecl _set_new_handler( _PNH );

    void	xrDebug::_initialize		(const bool &dedicated)
    {
		handler							= 0;
        _set_new_mode					(1);					// gen exception if can't allocate memory
        _set_new_handler				(_out_of_memory	);		// exception-handler for 'out of memory' condition
		std::set_terminate				(_terminate);
		std::set_unexpected				(_terminate);
        ::SetUnhandledExceptionFilter	( UnhandledFilter );	// exception handler to all "unhandled" exceptions
    }

#endif

#endif

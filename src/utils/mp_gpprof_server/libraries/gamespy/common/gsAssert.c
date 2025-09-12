///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "gsCommon.h"
#include "gsDebug.h"



// This is the platform specific default assert condition handler
extern void  _gsDebugAssert	(const char * string);

static gsDebugAssertCallback gsDebugAssertHandler = _gsDebugAssert;

//  Call this function to override the default assert handler  
//	New function should render message / log message based on string passed
void gsDebugAssertCallbackSet(gsDebugAssertCallback theCallback)
{
	if (theCallback)
		gsDebugAssertHandler = theCallback;
	else
		gsDebugAssertHandler = _gsDebugAssert;
}


// This is the default assert condition handler
void  gsDebugAssert				(const char *szError,const char *szText, const char *szFile, int line)
{
	char String[256];
	// format into buffer 
	sprintf(&String[0], szError,szText,szFile,line);

	// call plat specific handler
	(*gsDebugAssertHandler)(String);

}


// ****************************************************
// Todo: move to platform specific modules
#ifdef _XBOX
	// ErrorMessage: Displays message and goes into an infinite loop
	// continues rendering
	void  _gsDebugAssert			(const char *string)
	{
		//DebugBreak();
		OutputDebugString( string);
		exit(0);
	}

#elif defined _WIN32
	#include <windows.h>
	#pragma warning(disable: 4127) // disable warnings from "conditional expression is constant"

	// ErrorMessage: Displays message and goes into an infinite loop
	// continues rendering
	void  _gsDebugAssert			(const char *string)
	{

		//DebugBreak();
		

		#ifdef _CONSOLE   //,_MBCS

			printf("%s",string);
			while(1)
			{
			};

		#else
		{		
			OutputDebugStringA( string);

			#ifndef GS_ASSERT_NO_MESSAGEBOX
			{
				int rcode = MessageBoxA(NULL,string,"Assert Failed",MB_ABORTRETRYIGNORE|MB_ICONHAND|MB_SETFOREGROUND|MB_TASKMODAL);

				if(rcode == IDABORT)
				{
					exit(0);
				}
				if(rcode == IDRETRY)
				{
					DebugBreak();
					return;
				}
			}
			#else
				DebugBreak();
			#endif
		}
		#endif
	}

#elif defined _PSP
	// ToDo:
	// ErrorMessage: Displays message and goes into an infinite loop
	// continues rendering
	void  _gsDebugAssert			(const char *string)
	{
		printf(string);
		
		while(1)
		{
		};

	}


#elif defined _PS2

	// already included in gsPlatform.h
	/*
	#include <eetypes.h>
	#include <eekernel.h>
	*/

	// ErrorMessage: Displays message and goes into an infinite loop
	// continues rendering
	void  _gsDebugAssert			(const char *string)
	{

		scePrintf(string);
		while(1);;
	}



#elif defined _MACOSX
	void  _gsDebugAssert			(const char *string)
	{
		printf(string);
		
		while(1)
		{
		};

	}


#elif defined _LINUX
	void  _gsDebugAssert			(const char *string)
	{
		printf(string);
		
		while(1)
		{
		};

	}


#elif defined _NITRO
	void  _gsDebugAssert			(const char *string)
	{
#if SDK_FINALROM != 1
		OS_TPanic("%s",string);
#else
		GSI_UNUSED(string);
#endif
	}

#elif defined(_REVOLUTION)
	void _gsDebugAssert(const char *string)
	{
		OSHalt(string);
		
	}
#else
	// ErrorMessage: Displays message and goes into an infinite loop
	// continues rendering
	void  _gsDebugAssert			(const char *string)
	{

		printf(string);
		while(1);;

	}

#endif





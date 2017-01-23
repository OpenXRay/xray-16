/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2000 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

#pragma once

#include <windows.h>
#include <DbgHelp.h>

/*//////////////////////////////////////////////////////////////////////
Type Definitions
//////////////////////////////////////////////////////////////////////*/
// The type for the filter function called by the Crash Handler API.
typedef LONG(__stdcall *PFNCHFILTFN) (EXCEPTION_POINTERS* pExPtrs);

/*//////////////////////////////////////////////////////////////////////
Crash Handler Function Definitions
//////////////////////////////////////////////////////////////////////*/

/*----------------------------------------------------------------------
FUNCTION        :   GetFirstStackTraceString
GetNextStackTraceString
DISCUSSION      :
These functions allow you to get the stack trace information for a
crash.  Call GetFirstStackTraceString and then GetNextStackTraceString
to get the entire stack trace for a crash.
The options GSTSO_PARAMS, GSTSO_MODULE, GSTSO_SYMBOL, and
GSTSO_SRCLINE, appear in that order in the string.
PARAMETERS      :
dwOpts   - The options flags  "Or" the following options together.
0             - Just put the PC address in the string.
GSTSO_PARAMS  - Include the possible params.
GSTSO_MODULE  - Include the module name as well.
GSTSO_SYMBOL  - Include the symbol name of the stack
address.
GSTSO_SRCLINE - Include source and line info of the
stack address.
pExtPtrs - The exception pointers passed to the crash handler
function.
RETURNS         :
!NULL - The requested stack trace string.
NULL  - There was a problem.
----------------------------------------------------------------------*/
#define GSTSO_PARAMS    0x01
#define GSTSO_MODULE    0x02
#define GSTSO_SYMBOL    0x04
#define GSTSO_SRCLINE   0x08
LPCTSTR __stdcall GetFirstStackTraceString(DWORD dwOpts, EXCEPTION_POINTERS* pExPtrs);
LPCTSTR __stdcall GetNextStackTraceString(DWORD dwOpts, EXCEPTION_POINTERS* pExPtrs);

/*----------------------------------------------------------------------
FUNCTION        :   GetFirstStackTraceStringVB
GetNextStackTraceStringVB
DISCUSSION      :
The VB wrappers on GetFirstStackTraceString and
GetNextStackTraceString since VB cannot handle returning a string from
a DLL call.
PARAMETERS      :
dwOpts   - The options flags  "Or" the following options together.
0             - Just put the PC address in the string.
GSTSO_PARAMS  - Include the possible params.
GSTSO_MODULE  - Include the module name as well.
GSTSO_SYMBOL  - Include the symbol name the stack
address.
GSTSO_SRCLINE - Include source and line info for the
address.
pExtPtrs - The exception pointers passed to the crash handler
function.
szBuff   - The output buffer.
uiSize   - The size of the output buffer.
RETURNS         :
TRUE  - The string was copied into szBuff.
FALSE - There was a problem.
----------------------------------------------------------------------*/
BOOL __stdcall GetFirstStackTraceStringVB(DWORD dwOpts, EXCEPTION_POINTERS* pExPtrs, LPTSTR szBuff, UINT uiSize);
BOOL __stdcall GetNextStackTraceStringVB(DWORD dwOpts, EXCEPTION_POINTERS * pExPtrs, LPTSTR szBuff, UINT uiSize);
/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2000 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

#ifndef _CRASHHANDLER_H
#define _CRASHHANDLER_H

#ifdef __cplusplus
extern "C" {
#endif  //__cplusplus

/*//////////////////////////////////////////////////////////////////////
                            Type Definitions
//////////////////////////////////////////////////////////////////////*/
// The type for the filter function called by the Crash Handler API.
typedef LONG ( __stdcall *PFNCHFILTFN ) ( EXCEPTION_POINTERS * pExPtrs ) ;

/*//////////////////////////////////////////////////////////////////////
                   Crash Handler Function Definitions
//////////////////////////////////////////////////////////////////////*/

/*----------------------------------------------------------------------
FUNCTION        :   SetCrashHandlerFilter
DISCUSSION      :
    Sets the filter function that will be called when there is a fatal
crash.  The passed in function will only be called if the crash is one
of the modules passed to AddCrashHandlerLimitModule.  If no modules have
been added to narrow down the interested modules then the callback
filter function will always be called.
PARAMETERS      :
    pFn - A valid code pointer.  If this is NULL, then the Crash Handler
          filter function is removed.
RETURNS         :
    1 - The crash handler was properly set.
    0 - There was a problem.
----------------------------------------------------------------------*/
BOOL  __stdcall
                             SetCrashHandlerFilter ( PFNCHFILTFN pFn ) ;

/*----------------------------------------------------------------------
FUNCTION        :   AddCrashHandlerLimitModule
DISCUSSION      :
    Adds a module to the list of modules that CrashHandler will call the
callbeack function for.  If no modules are added, then the callback is
called for all crashes.  Limiting the specific modules allows the crash
handler to be installed for just the modules you are responsible for.
PARAMETERS      :
    hMod - The HMODULE or HINSTANCE.
RETURNS         :
    1 - The module was added.
    0 - There was a problem.
----------------------------------------------------------------------*/
BOOL  __stdcall
                           AddCrashHandlerLimitModule ( HMODULE hMod ) ;

/*----------------------------------------------------------------------
FUNCTION        :   GetLimitModuleCount
DISCUSSION      :
    Returns the number of limit modules for the crash handler.
PARAMETERS      :
    None.
RETURNS         :
    The item count.  This can be zero if not limiting modules have been
added.
----------------------------------------------------------------------*/
UINT  __stdcall GetLimitModuleCount ( void ) ;

/*----------------------------------------------------------------------
FUNCTION        :   GetLimitModulesArray
DISCUSSION      :
    Returns the limit modules currently active.
PARAMETERS      :
    pahMod - The array that the active limit modules are copied into.
    uiSize - The size of ahMod in items.
RETURNS         :
    Note:  Since VB blows away the last error value, this function
           returns values indicating what went wrong.
    GLMA_SUCCESS        - The limit items where copied.
    GLMA_BADPARAM       - The parameter(s) were invalid.
    GLMA_BUFFTOOSMALL   - The ahMod was too small to copy all the
                          values.
    GLMA_FAILURE        - There was a major problem.
----------------------------------------------------------------------*/
#define GLMA_SUCCESS        1
#define GLMA_BADPARAM       -1
#define GLMA_BUFFTOOSMALL   -2
#define GLMA_FAILURE        0
int  __stdcall
               GetLimitModulesArray ( HMODULE * pahMod , UINT uiSize ) ;

/*//////////////////////////////////////////////////////////////////////
          EXCEPTION_POINTER Translation Functions Declarations
//////////////////////////////////////////////////////////////////////*/

/*----------------------------------------------------------------------
FUNCTION        :   GetFaultReason
DISCUSSION      :
    Returns a string that describes the fault that occured.  The return
string looks similar to the string returned by Win95's fault dialog.
The returned buffer is constant and do not change it.
    This function can only be called from the callback.
PARAMETERS      :
    pExPtrs - The exeption pointers passed to the callback.
RETURNS         :
    !NULL - The constant string that describes the fault.
    NULL  - There was a problem translating the string.
----------------------------------------------------------------------*/
LPCTSTR  __stdcall
                       GetFaultReason ( EXCEPTION_POINTERS * pExPtrs ) ;

/*----------------------------------------------------------------------
FUNCTION        :   GetFaultReasonVB
DISCUSSION      :
    The VB wrapper for GetFaultReason.  This calls GetFaultReason and
copys the resulting string into the passed in buffer.
PARAMETERS      :
    pExPtrs - The exeption pointers passed to the callback.
    szBuff  - The output buffer.
    uiSize  - The size of the output buffer.
RETURNS         :
    TRUE  - The buffer is filled.
    FALSE - There was a problem.
----------------------------------------------------------------------*/
BOOL  __stdcall
                       GetFaultReasonVB ( EXCEPTION_POINTERS * pExPtrs ,
                                          LPTSTR               szBuff  ,
                                          UINT                 uiSize );

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
LPCTSTR  __stdcall
             GetFirstStackTraceString ( DWORD                dwOpts  ,
                                        EXCEPTION_POINTERS * pExPtrs  );
LPCTSTR  __stdcall
             GetNextStackTraceString ( DWORD                dwOpts  ,
                                       EXCEPTION_POINTERS * pExPtrs  ) ;

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
BOOL  __stdcall
           GetFirstStackTraceStringVB ( DWORD                dwOpts  ,
                                        EXCEPTION_POINTERS * pExPtrs ,
                                        LPTSTR               szBuff  ,
                                        UINT                 uiSize   );
BOOL  __stdcall
           GetNextStackTraceStringVB ( DWORD                dwOpts  ,
                                       EXCEPTION_POINTERS * pExPtrs ,
                                       LPTSTR               szBuff  ,
                                       UINT                 uiSize   );

/*----------------------------------------------------------------------
FUNCTION        :   GetRegisterString
DISCUSSION      :
    Returns a string with all the registers and their values.  This
function hides all the platform differences.
PARAMETERS      :
    pExtPtrs - The exception pointers passed to the crash handler
               function.
RETURNS         :
    !NULL - The requested register string.
    NULL  - There was a problem.
----------------------------------------------------------------------*/
LPCTSTR  __stdcall
              GetRegisterString ( EXCEPTION_POINTERS * pExPtrs ) ;

/*----------------------------------------------------------------------
FUNCTION        :   GetRegisterStringVB
DISCUSSION      :
    Handles GetRegisterString for VB.
PARAMETERS      :
    pExtPtrs - The exception pointers passed to the crash handler
               function.
    szBuff   - The output buffer.
    uiSize   - The size of the output buffer.
RETURNS         :
    TRUE  - The string was copied into szBuff.
    FALSE - There was a problem.
----------------------------------------------------------------------*/
BOOL  __stdcall
              GetRegisterStringVB ( EXCEPTION_POINTERS * pExPtrs ,
                                    LPTSTR               szBuff  ,
                                    UINT                 uiSize   ) ;

#ifdef __cplusplus
}
#endif  //__cplusplus

#endif  // _CRASHHANDLER_H



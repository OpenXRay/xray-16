/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2000 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

#ifndef _INTERNAL_H
#define _INTERNAL_H

// The NT4 specific version of GetLoadedModules.
BOOL __stdcall NT4GetLoadedModules ( DWORD     dwPID        ,
                                     UINT      uiCount      ,
                                     HMODULE * paModArray   ,
                                     LPDWORD   pdwRealCount   ) ;

// The TOOLHELP32 specific version of GetLoadedModules.
BOOL __stdcall TLHELPGetLoadedModules ( DWORD     dwPID        ,
                                        UINT      uiCount      ,
                                        HMODULE * paModArray   ,
                                        LPDWORD   pdwRealCount   ) ;

// The NT version of GetModuleBaseName.
DWORD __stdcall NTGetModuleBaseName ( HANDLE  hProcess   ,
                                      HMODULE hModule    ,
                                      LPTSTR  lpBaseName ,
                                      DWORD   nSize       ) ;


// The TOOLHELP32 specific version of GetModuleFilenameEx
DWORD __stdcall TLHELPGetModuleFileNameEx ( DWORD     dwPID        ,
                                            HANDLE    hProcess     ,
                                            HMODULE   hModule      ,
                                            LPTSTR    szFilename   ,
                                            DWORD     nSize         ) ;

// The NT version of GetModuleFilenameEx.
DWORD __stdcall NTGetModuleFileNameEx ( DWORD     dwPID        ,
                                        HANDLE    hProcess     ,
                                        HMODULE   hModule      ,
                                        LPTSTR    szFilename   ,
                                        DWORD     nSize         ) ;



// A helper function to get the import descriptor for a the specified
// module.
PIMAGE_IMPORT_DESCRIPTOR
                     GetNamedImportDescriptor ( HMODULE hModule     ,
                                                LPCSTR  szImportMod  ) ;

// A useful macro.
#define MakePtr( cast , ptr , AddValue ) \
                                 (cast)( (DWORD)(ptr)+(DWORD)(AddValue))


#ifdef _DEBUG
// The function that initializes the MemStress system during DllMain's
// process attach.
BOOL InternalMemStressInitialize ( void ) ;
// The function that shuts down the MemStress system during DllMain's
// process detach.
BOOL InternalMemStressShutdown ( void ) ;
#endif  // _DEBUG


#endif  // _INTERNAL_H



@echo off

setlocal
setlocal enabledelayedexpansion

set null_args=0
if [%1]==[] set null_args=1
if [%2]==[] set null_args=1
if [%3]==[] set null_args=1
if [%4]==[] set null_args=1
if !null_args!==1 (
  echo usage: %0 ^<game_installation_dir^> ^<repository_root_dir^> {Win32^|Win64} {Debug^|Mixed^|Release}
  goto ret
)
if not exist %1 (
  echo path not found: %1
  goto ret
)
if not exist %2 (
  echo path not found: %2
  goto ret
)
set platform=%3
if %platform%==Win32 (
  goto platform_ok
)
if %platform%==Win64 (
  goto platform_ok
)
echo invalid platform: %platform%
goto ret

:platform_ok
set cfg=%4
if %cfg%==Debug (
  set dst=%1
  set dst=!dst:"=!\_bin_dbg
  goto cfg_ok
)
if %cfg%==Mixed (
  set dst=%1
  set dst=!dst:"=!\_bin_mix
  goto cfg_ok
)
if %cfg%==Release (
  set dst=%1
  set dst=!dst:"=!\_bin_rel
  goto cfg_ok
)
echo invalid configuration: %cfg%
goto ret

:cfg_ok
set dst=!dst!_%platform%
set src=%2
set src=!src:"=!\bin\%platform%\%cfg%

where cp >nul 2>nul
if %errorLevel% neq 0 (
  set cp_tool=copy /Y
) else (
  set cp_tool=cp
)

call :COPY_FILE xrEngine.exe
call :COPY_FILE xrAPI.dll
call :COPY_FILE xrCore.dll
call :COPY_FILE xrCDB.dll
call :COPY_FILE xrSound.dll
call :COPY_FILE xrParticles.dll
call :COPY_FILE xrPhysics.dll
call :COPY_FILE xrD3D9-Null.dll
call :COPY_FILE xrAICore.dll
call :COPY_FILE xrScriptEngine.dll
call :COPY_FILE xrGame.dll
call :COPY_FILE xrGameSpy.dll
call :COPY_FILE xrNetServer.dll
call :COPY_FILE xrRender_R1.dll
call :COPY_FILE xrRender_R2.dll
call :COPY_FILE xrRender_R3.dll
call :COPY_FILE xrRender_R4.dll
call :COPY_FILE xrRender_GL.dll
if %platform%==Win32 (
  call :COPY_FILE amd_ags_x86.dll
)
if %platform%==Win64 (
  call :COPY_FILE amd_ags_x64.dll
)
rem CxImage is compiled as DLLs only in debug configuration
if %cfg%==Debug (
  call :COPY_FILE CxImage.dll
)
call :COPY_FILE LuaJIT.dll
call :COPY_FILE luabind.dll
call :COPY_FILE ODE.dll
call :COPY_FILE OpenAL32.dll
%cp_tool% "!src!\OpenAL32.dll" "!dst!\dedicated\OpenAL32.dll"
if exist "!src!_Dedicated\dedicated\xrEngine.exe" (
  %cp_tool% "!src!_Dedicated\dedicated\xrEngine.exe" "!dst!\dedicated\xrEngine.exe"
)
call :COPY_FILE xrWeatherEditor.dll
call :COPY_FILE xrManagedApi.dll
call :COPY_FILE xrSdkControls.dll
call :COPY_FILE xrPostprocessEditor.exe

goto :EOF

:COPY_FILE
if exist "!src!\%1" (
%cp_tool% "!src!\%1" "!dst!\"
)
goto :EOF

:ret
endlocal

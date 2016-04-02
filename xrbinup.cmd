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

%cp_tool% "!src!\xrEngine.exe" "!dst!\xrEngine.exe"
%cp_tool% "!src!\xrAPI.dll" "!dst!\xrAPI.dll"
%cp_tool% "!src!\xrCore.dll" "!dst!\xrCore.dll"
%cp_tool% "!src!\xrCDB.dll" "!dst!\xrCDB.dll"
%cp_tool% "!src!\xrSound.dll" "!dst!\xrSound.dll"
%cp_tool% "!src!\xrParticles.dll" "!dst!\xrParticles.dll"
%cp_tool% "!src!\xrPhysics.dll" "!dst!\xrPhysics.dll"
%cp_tool% "!src!\xrD3D9-Null.dll" "!dst!\xrD3D9-Null.dll"
%cp_tool% "!src!\xrAICore.dll" "!dst!\xrAICore.dll"
%cp_tool% "!src!\xrScriptEngine.dll" "!dst!\xrScriptEngine.dll"
%cp_tool% "!src!\xrGame.dll" "!dst!\xrGame.dll"
%cp_tool% "!src!\xrGameSpy.dll" "!dst!\xrGameSpy.dll"
%cp_tool% "!src!\xrNetServer.dll" "!dst!\xrNetServer.dll"
%cp_tool% "!src!\xrRender_R1.dll" "!dst!\xrRender_R1.dll"
%cp_tool% "!src!\xrRender_R2.dll" "!dst!\xrRender_R2.dll"
%cp_tool% "!src!\xrRender_R3.dll" "!dst!\xrRender_R3.dll"
%cp_tool% "!src!\xrRender_R4.dll" "!dst!\xrRender_R4.dll"
%cp_tool% "!src!\xrRender_GL.dll" "!dst!\xrRender_GL.dll"
if %platform%==Win32 (
  %cp_tool% "!src!\amd_ags_x86.dll" "!dst!\amd_ags_x86.dll"
)
if %platform%==Win64 (
  %cp_tool% "!src!\amd_ags_x64.dll" "!dst!\amd_ags_x64.dll"
)
rem CxImage is compiled as DLLs only in debug configuration
if %cfg%==Debug (
  %cp_tool% "!src!\CxImage.dll" "!dst!\CxImage.dll"
)
%cp_tool% "!src!\LuaJIT.dll" "!dst!\LuaJIT.dll"
%cp_tool% "!src!\luabind.dll" "!dst!\luabind.dll"
%cp_tool% "!src!\ODE.dll" "!dst!\ODE.dll"
%cp_tool% "!src!\OpenAL32.dll" "!dst!\OpenAL32.dll"
%cp_tool% "!src!\OpenAL32.dll" "!dst!\dedicated\OpenAL32.dll"
%cp_tool% "!src!_Dedicated\dedicated\xrEngine.exe" "!dst!\dedicated\xrEngine.exe"
%cp_tool% "!src!\xrWeatherEditor.dll" "!dst!\xrWeatherEditor.dll"
%cp_tool% "!src!\xrManagedApi.dll" "!dst!\xrManagedApi.dll"
%cp_tool% "!src!\xrSdkControls.dll" "!dst!\xrSdkControls.dll"
%cp_tool% "!src!\xrPostprocessEditor.exe" "!dst!\xrPostprocessEditor.exe"

:ret
endlocal

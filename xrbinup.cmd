@echo off

setlocal
setlocal enabledelayedexpansion

set null_args=0
if [%1]==[] set null_args=1
if [%2]==[] set null_args=1
if [%3]==[] set null_args=1
if !null_args!==1 (
  echo usage: %0 ^<game_installation_dir^> ^<repository_root_dir^> {dbg^|mix^|rel}
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
set cfg=%3
set cfg_valid=0
if %cfg%==dbg set cfg_valid=1
if %cfg%==mix set cfg_valid=1
if %cfg%==rel set cfg_valid=1
if !cfg_valid!==1 goto args_validated
echo invalid configuration: %cfg%
goto ret

:args_validated
set src=%2\bin
if %cfg%==dbg (
  set cfg_long=Debug
  set dst=%1\_bin_dbg
  set dbg=1
) else if %cfg%==mix (
  set cfg_long=Mixed
  set dst=%1\_bin_mix
) else if %cfg%==rel (
  set cfg_long=Release
  set dst=%1\_bin_rel
)

set cp_tool=cp

%cp_tool% %src%\%cfg_long%\xrEngine.exe %dst%\xrEngine.exe
%cp_tool% %src%\%cfg_long%\xrAPI.dll %dst%\xrAPI.dll
%cp_tool% %src%\%cfg_long%\xrCore.dll %dst%\xrCore.dll
%cp_tool% %src%\%cfg_long%\xrCDB.dll %dst%\xrCDB.dll
%cp_tool% %src%\%cfg_long%\xrSound.dll %dst%\xrSound.dll
%cp_tool% %src%\%cfg_long%\xrParticles.dll %dst%\xrParticles.dll
%cp_tool% %src%\%cfg_long%\xrPhysics.dll %dst%\xrPhysics.dll
%cp_tool% %src%\%cfg_long%\xrD3D9-Null.dll %dst%\xrD3D9-Null.dll
%cp_tool% %src%\%cfg_long%\xrAICore.dll %dst%\xrAICore.dll
%cp_tool% %src%\%cfg_long%\xrScriptEngine.dll %dst%\xrScriptEngine.dll
%cp_tool% %src%\%cfg_long%\xrGame.dll %dst%\xrGame.dll
%cp_tool% %src%\%cfg_long%\xrGameSpy.dll %dst%\xrGameSpy.dll
%cp_tool% %src%\%cfg_long%\xrNetServer.dll %dst%\xrNetServer.dll
%cp_tool% %src%\%cfg_long%\xrRender_R1.dll %dst%\xrRender_R1.dll
%cp_tool% %src%\%cfg_long%\xrRender_R2.dll %dst%\xrRender_R2.dll
%cp_tool% %src%\%cfg_long%\xrRender_R3.dll %dst%\xrRender_R3.dll
%cp_tool% %src%\%cfg_long%\xrRender_R4.dll %dst%\xrRender_R4.dll
%cp_tool% %src%\%cfg_long%\xrXMLParser.dll %dst%\xrXMLParser.dll
rem CxImage is compiled as DLLs only in debug configuration
if defined dbg (
  %cp_tool% %src%\%cfg_long%\CxImage.dll %dst%\CxImage.dll
)
%cp_tool% %src%\%cfg_long%\Lua.JIT.1.1.4.dll %dst%\Lua.JIT.1.1.4.dll
%cp_tool% %src%\%cfg_long%\LuaBind.beta7-Devel.RC4.dll %dst%\LuaBind.beta7-Devel.RC4.dll
%cp_tool% %src%\%cfg_long%\ODE.dll %dst%\ODE.dll
%cp_tool% %src%\%cfg_long%\OpenAL32.dll %dst%\OpenAL32.dll
%cp_tool% %src%\%cfg_long%\OpenAL32.dll %dst%\dedicated\OpenAL32.dll
%cp_tool% %src%\%cfg_long%_Dedicated\dedicated\xrEngine.exe %dst%\dedicated\xrEngine.exe
%cp_tool% %src%\%cfg_long%\xrWeatherEditor.dll %dst%\xrWeatherEditor.dll
%cp_tool% %src%\%cfg_long%\xrManagedApi.dll %dst%\xrManagedApi.dll
%cp_tool% %src%\%cfg_long%\xrSdkControls.dll %dst%\xrSdkControls.dll
%cp_tool% %src%\%cfg_long%\xrPostprocessEditor.exe %dst%\xrPostprocessEditor.exe

:ret
endlocal

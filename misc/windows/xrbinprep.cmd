@echo off

setlocal
setlocal enabledelayedexpansion
rem mklink requires admin; check or exit
net session >nul 2>&1
if %errorLevel% neq 0 (
  echo This script requires administrator privileges.
  goto ret
)
if [%1]==[] (
  echo usage: %0 ^<game_installation_dir^>
  goto ret
)
set game_root=%1
if not exist %game_root% (
  echo path not found: %game_root%
  goto ret
)
set game_root=!game_root:"=!
for /f "delims=" %%a in ('chdir') do set git_root=%%a
pushd %game_root%
set bin_dbg_win32=_bin_dbg_Win32
set bin_mix_win32=_bin_mix_Win32
set bin_rel_win32=_bin_rel_Win32
set xbin=%bin_dbg_win32% %bin_mix_win32% %bin_rel_win32%

where cp >nul 2>nul
if %errorLevel% neq 0 (
  set cp_tool=copy /Y
) else (
  set cp_tool=cp
)

for %%b in (%xbin%) do (
  if not exist %%b (
    mkdir %%b
  )
  if not exist %%b\dedicated (
    mkdir %%b\dedicated
  )
  %cp_tool% bin\dbghelp.dll %%b\dbghelp.dll
  %cp_tool% bin\eax.dll %%b\eax.dll
  %cp_tool% bin\wrap_oal.dll %%b\wrap_oal.dll
  %cp_tool% bin\eax.dll %%b\dedicated\eax.dll
  %cp_tool% bin\wrap_oal.dll %%b\dedicated\wrap_oal.dll
  %cp_tool% "%git_root%\src\Externals\OpenSSL\bin\libeay32.dll" %%b\libeay32.dll
  %cp_tool% "%git_root%\src\Externals\OpenSSL\bin\ssleay32.dll" %%b\ssleay32.dll
)
%cp_tool% "%git_root%\src\Externals\BugTrap\bin\BugTrapD.dll" %bin_dbg_win32%\BugTrap.dll
%cp_tool% "%git_root%\src\Externals\BugTrap\bin\BugTrap.dll" %bin_mix_win32%\BugTrap.dll
%cp_tool% "%git_root%\src\Externals\BugTrap\bin\BugTrap.dll" %bin_rel_win32%\BugTrap.dll
if exist gamedata (
  echo gamedata already exists. Remove/rename it, then
  echo   create soft link to "%git_root%\res\gamedata"
  echo   -or-
  echo   run this script again
) else (
  mklink /D gamedata "%git_root%\res\gamedata"
)
popd
call :make_binup Win32 Debug
call :make_binup Win32 Mixed
call :make_binup Win32 Release
goto ret

rem args: <platform> <configuration>
:make_binup
setlocal
set platform=%~1
set conf=%~2
set fname=.xrbinup_%conf%_%platform%.cmd
( ^
echo @echo off&& ^
echo setlocal&& ^
echo set src=%git_root%&& ^
echo set dst=!game_root!&& ^
echo call xrbinup.cmd "%%dst%%" "%%src%%" %platform% %conf%&& ^
echo endlocal) ^
> %fname%
endlocal
goto:eof

:ret
endlocal

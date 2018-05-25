REM based on https://github.com/bluebird75/luaunit/blob/80e5b37e0f3e0016dcf7dff0004a4d065cd4ee61/.appveyor/install-lua.cmd
REM BSD license: https://github.com/bluebird75/luaunit/blob/7382208f9e7ff433e0bf3feeb1990bcab1de9877/LICENSE.txt

REM This is a batch file to help with setting up the desired Lua environment.
REM It is intended to be run as "install" step from within AppVeyor.

REM version numbers and file names for binaries from http://sf.net/p/luabinaries/
set VER_51=5.1.5
set VER_52=5.2.4
set VER_53=5.3.3
set ZIP_51=lua-%VER_51%_Win32_bin.zip
set ZIP_52=lua-%VER_52%_Win32_bin.zip
set ZIP_53=lua-%VER_53%_Win32_bin.zip
set ZIP_51_DEV=lua-%VER_51%_Win32_dll15_lib.zip

:cinst
@echo off
if NOT "%LUAENV%"=="cinst" goto lua51
echo Chocolatey install of Lua ...
if NOT EXIST "C:\Program Files (x86)\Lua\5.1\lua.exe" (
    @echo on
    cinst lua
) else (
    @echo on
    echo Using cached version of Lua
)
set LUA="C:\Program Files (x86)\Lua\5.1\lua.exe"
@echo off
goto :EOF

:lua51
@echo off
if NOT "%LUAENV%"=="lua51" goto lua52
echo Setting up Lua 5.1 ...
if NOT EXIST "lua51\lua5.1.exe" (
    @echo on
    echo Fetching Lua v5.1 from internet
    curl -fLsS -o %ZIP_51% http://sourceforge.net/projects/luabinaries/files/%VER_51%/Tools%%20Executables/%ZIP_51%/download
    unzip -d lua51 %ZIP_51%
) else (
    echo Using cached version of Lua v5.1
)
if NOT EXIST "lua51\lua5.1.lib" (
    @echo on
    echo Fetching Lua_DEV v5.1 from internet
    curl -fLsS -o %ZIP_51_DEV% http://sourceforge.net/projects/luabinaries/files/%VER_51%/Windows%20Libraries/Dynamic/%ZIP_51_DEV%/download
    unzip -d lua51 %ZIP_51_DEV%
) else (
    echo Using cached version of Lua_DEV v5.1
)
set LUA=lua51\lua5.1.exe
@echo off
goto :EOF

:lua52
@echo off
if NOT "%LUAENV%"=="lua52" goto lua53
echo Setting up Lua 5.2 ...
if NOT EXIST "lua52\lua52.exe" (
    @echo on
    echo Fetching Lua v5.2 from internet
    curl -fLsS -o %ZIP_52% http://sourceforge.net/projects/luabinaries/files/%VER_52%/Tools%%20Executables/%ZIP_52%/download
    unzip -d lua52 %ZIP_52%
) else (
    echo Using cached version of Lua v5.2
)
@echo on
set LUA=lua52\lua52.exe
@echo off
goto :EOF

:lua53
@echo off
if NOT "%LUAENV%"=="lua53" goto luajit
echo Setting up Lua 5.3 ...
if NOT EXIST "lua53\lua53.exe" (
    @echo on
    echo Fetching Lua v5.3 from internet
    curl -fLsS -o %ZIP_53% http://sourceforge.net/projects/luabinaries/files/%VER_53%/Tools%%20Executables/%ZIP_53%/download
    unzip -d lua53 %ZIP_53%
) else (
    echo Using cached version of Lua v5.3
)
@echo on
set LUA=lua53\lua53.exe
@echo off
goto :EOF

:luajit
if NOT "%LUAENV%"=="luajit20" goto luajit21
echo Setting up LuaJIT 2.0 ...
if NOT EXIST "luajit20\luajit.exe" (
    call %~dp0install-luajit.cmd LuaJIT-2.0.4 luajit20
) else (
    echo Using cached version of LuaJIT 2.0
)
set LUA=luajit20\luajit.exe
goto :EOF

:luajit21
echo Setting up LuaJIT 2.1 ...
if NOT EXIST "luajit21\luajit.exe" (
    call %~dp0install-luajit.cmd LuaJIT-2.1.0-beta2 luajit21
) else (
    echo Using cached version of LuaJIT 2.1
)
set LUA=luajit21\luajit.exe

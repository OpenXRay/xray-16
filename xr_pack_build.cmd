if "%CONFIGURATION%"=="Debug" (
    if %PLATFORM%==x86 (
        set PLATFORM_FOLDER=Win32
        set EDITION_NAME=Debug 32-bit
        goto :START
    )
)

if "%CONFIGURATION%"=="Debug" (
    if %PLATFORM%==x64 (
        set PLATFORM_FOLDER=Win64
        set EDITION_NAME=Debug 64-bit
        goto :START
    )
)

if "%CONFIGURATION%"=="Mixed" (
    if %PLATFORM%==x86 (
        set PLATFORM_FOLDER=Win32
        set EDITION_NAME=Mixed 32-bit
        goto :START
    )
)

if "%CONFIGURATION%"=="Mixed" (
    if %PLATFORM%==x64 (
        set PLATFORM_FOLDER=Win64
        set EDITION_NAME=Mixed 64-bit
        goto :START
    )
)

if "%CONFIGURATION%"=="Release" (
    if %PLATFORM%==x86 (
        set PLATFORM_FOLDER=Win32
        set EDITION_NAME=Release 32-bit
        goto :START
    )
)

if "%CONFIGURATION%"=="Release" (
    if %PLATFORM%==x64 (
        set PLATFORM_FOLDER=Win64
        set EDITION_NAME=Release 64-bit
        goto :START
    )
)

if "%CONFIGURATION%"=="Release Master Gold" (
    if %PLATFORM%==x86 (
        set PLATFORM_FOLDER=Win32
        set EDITION_NAME=Gold 32-bit
        goto :START
    )
)

if "%CONFIGURATION%"=="Release Master Gold" (
    if %PLATFORM%==x64 (
        set PLATFORM_FOLDER=Win64
        set EDITION_NAME=Gold 64-bit
        goto :START
    )
)

echo ! Unknown configuration and/or platform
goto :EOF

:START
md res\bin\
md res\bin\utils
call :COPY_ENGINE

cd res
7z a "OpenXRay.%EDITION_NAME%.7z" * -xr!.* -xr!*.pdb -x!bin\utils
7z a "Symbols.%EDITION_NAME%.7z" bin\*.pdb -i!License.txt -i!README.md -xr!.*
7z a "Utils.%EDITION_NAME%.7z" bin\utils\* -i!License.txt -i!README.md -xr!.*
cd ..

rem Return edition name
set NEED_OUTPUT=%1
if defined NEED_OUTPUT (
    set %~1=%EDITION_NAME%
)
goto :EOF

:COPY_ENGINE
copy "bin\%PLATFORM_FOLDER%\%CONFIGURATION%\*.dll" res\bin\
copy "bin\%PLATFORM_FOLDER%\%CONFIGURATION%\*.exe" res\bin\
copy "bin\%PLATFORM_FOLDER%\%CONFIGURATION%\*.pdb" res\bin\
copy "bin\%PLATFORM_FOLDER%\%CONFIGURATION%\utils\*" res\bin\utils\
copy License.txt res\
copy README.md res\
goto :EOF

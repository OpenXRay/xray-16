if %CONFIGURATION%==Debug (
    if %PLATFORM%==x86 (
        set PLATFORM=Win32
        set SHORT_NAME=Dx86
        goto :START
    )
)

if %CONFIGURATION%==Debug (
    if %PLATFORM%==x64 (
        set PLATFORM=Win64
        set SHORT_NAME=Dx64
        goto :START
    )
)

if %CONFIGURATION%==Mixed (
    if %PLATFORM%==x86 (
        set PLATFORM=Win32
        set SHORT_NAME=Mx86
        goto :START
    )
)

if %CONFIGURATION%==Mixed (
    if %PLATFORM%==x64 (
        set PLATFORM=Win64
        set SHORT_NAME=Mx64
        goto :START
    )
)

if %CONFIGURATION%==Release (
    if %PLATFORM%==x86 (
        set PLATFORM=Win32
        set SHORT_NAME=Rx86
        goto :START
    )
)

if %CONFIGURATION%==Release (
    if %PLATFORM%==x64 (
        set PLATFORM=Win64
        set SHORT_NAME=Rx64
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
7z a OpenXRay.%SHORT_NAME%.7z * -xr!.* -xr!*.pdb -x!bin\utils
7z a Symbols.%SHORT_NAME%.7z bin\*.pdb -i!License.txt -i!README.md -xr!.*
7z a Utils.%SHORT_NAME%.7z bin\utils\* -i!License.txt -i!README.md -xr!.*
cd ..
goto :EOF

:COPY_ENGINE
copy bin\%PLATFORM%\%CONFIGURATION%\*.dll res\bin\
copy bin\%PLATFORM%\%CONFIGURATION%\*.exe res\bin\
copy bin\%PLATFORM%\%CONFIGURATION%\*.pdb res\bin\
copy bin\%PLATFORM%\%CONFIGURATION%\utils\* res\bin\utils\
copy License.txt res\
copy README.md res\
goto :EOF

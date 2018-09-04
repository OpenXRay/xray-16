md res\bin\

if %CONFIGURATION%==Debug if %PLATFORM%==x86 goto :DX86
if %CONFIGURATION%==Debug if %PLATFORM%==x64 goto :DX64
if %CONFIGURATION%==Mixed if %PLATFORM%==x86 goto :MX86
if %CONFIGURATION%==Mixed if %PLATFORM%==x64 goto :MX64
if %CONFIGURATION%==Release if %PLATFORM%==x86 goto :RX86
if %CONFIGURATION%==Release if %PLATFORM%==x64 goto :RX64

echo ! Unknown configuration and/or platform
goto :EOF

:DX86
cd bin\Win32\Debug
call :COPY_ENGINE
7z a OpenXRay.Dx86.7z .\*

cd ..\bin\Win32\Debug
call :COPY_SYMBOLS
7z a Symbols.Dx86.7z .\*
goto :EOF

:DX64
cd bin\Win64\Debug
call :COPY_ENGINE
7z a OpenXRay.Dx64.7z .\*

cd ..\bin\Win64\Debug
call :COPY_SYMBOLS
7z a Symbols.Dx64.7z .\*
goto :EOF

:MX86
cd bin\Win32\Mixed
call :COPY_ENGINE
7z a OpenXRay.Mx86.7z .\*

cd ..\bin\Win32\Mixed
call :COPY_SYMBOLS
7z a Symbols.Mx86.7z .\*
goto :EOF

:MX64
cd bin\Win64\Mixed
call :COPY_ENGINE
7z a OpenXRay.Mx64.7z .\*

cd ..\bin\Win64\Mixed
call :COPY_SYMBOLS
7z a Symbols.Mx64.7z .\*
goto :EOF

:RX86
cd bin\Win32\Release
call :COPY_ENGINE
7z a OpenXRay.Rx86.7z .\*

cd ..\bin\Win32\Release
call :COPY_SYMBOLS
7z a Symbols.Rx86.7z .\*
goto :EOF

:RX64
cd bin\Win64\Release
call :COPY_ENGINE
7z a OpenXRay.Rx64.7z .\*

cd ..\bin\Win64\Release
call :COPY_SYMBOLS
7z a Symbols.Rx64.7z .\*
goto :EOF

:COPY_ENGINE
copy *.dll ..\..\..\res\bin\
copy *.exe ..\..\..\res\bin\
cd ..\..\..\
copy Licence.txt .\res\
copy README.md .\res\
cd res\
goto :EOF

:COPY_SYMBOLS
copy *.pdb ..\..\..\res\bin\
cd ..\..\..\
copy Licence.txt .\res\
copy README.md .\res\
cd res\
goto :EOF

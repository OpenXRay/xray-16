md res\bins\ 

if %CONFIGURATION%==Debug if %PLATFORM%==x86 goto :DX86 
if %CONFIGURATION%==Debug if %PLATFORM%==x64 goto :DX64 
if %CONFIGURATION%==Mixed if %PLATFORM%==x86 goto :MX86 
if %CONFIGURATION%==Mixed if %PLATFORM%==x64 goto :MX64 
if %CONFIGURATION%==Mixed_COC if %PLATFORM%==x86 goto :MX_CoC86 
if %CONFIGURATION%==Mixed_COC if %PLATFORM%==x64 goto :MX_CoC64 
if %CONFIGURATION%==Release if %PLATFORM%==x86 goto :RX86 
if %CONFIGURATION%==Release if %PLATFORM%==x64 goto :RX64 
if %CONFIGURATION%==Release_COC if %PLATFORM%==x86 goto :RX_CoC86 
if %CONFIGURATION%==Release_COC if %PLATFORM%==x64 goto :RX_CoC64 

echo FAIL
goto :END 

:DX86 
cd _build\bin\Win32\Debug 
copy *.dll ..\..\..\..\res\bins\ 
copy *.exe ..\..\..\..\res\bins\ 
cd ..\ 
copy OpenXRay.exe ..\..\..\res\ 
cd ..\..\..\ 
copy License.txt .\res\ 
copy README.md .\res\ 
cd res\ 
rd /s /q gamedata_COC 
7z a xdOpenXRay.Dx86.7z .\* 
goto :END 

:DX64 
cd _build\bin\Win64\Debug 
copy *.dll ..\..\..\..\res\bins\ 
copy *.exe ..\..\..\..\res\bins\ 
cd ..\ 
copy OpenXRay.exe ..\..\..\res\ 
cd ..\..\..\ 
copy License.txt .\res\ 
copy README.md .\res\ 
cd res\ 
rd /s /q gamedata_COC 
7z a xdOpenXRay.Dx64.7z .\* 
goto :END 

:MX86 
cd _build\bin\Win32\Mixed 
copy *.dll ..\..\..\..\res\bins\ 
copy *.exe ..\..\..\..\res\bins\ 
cd ..\ 
copy OpenXRay.exe ..\..\..\res\ 
cd ..\..\..\ 
copy License.txt .\res\ 
copy README.md .\res\ 
cd res\ 
rd /s /q gamedata_COC 
7z a xdOpenXRay.Mx86.7z .\* 
goto :END 

:MX64 
cd _build\bin\Win64\Mixed 
copy *.dll ..\..\..\..\res\bins\ 
copy *.exe ..\..\..\..\res\bins\ 
cd ..\ 
copy OpenXRay.exe ..\..\..\res\ 
cd ..\..\..\ 
copy License.txt .\res\ 
copy README.md .\res\ 
cd res\ 
rd /s /q gamedata_COC 
7z a xdOpenXRay.Mx64.7z .\* 
goto :END 

:MX_CoC86 
cd _build\bin\Win32\Mixed_COC 
copy *.dll ..\..\..\..\res\bins\ 
copy *.exe ..\..\..\..\res\bins\ 
cd ..\ 
copy Stalker-CoC.exe ..\..\..\res\ 
cd ..\..\..\ 
copy License.txt .\res\ 
copy README.md .\res\ 
cd res\ 
rd /s /q gamedata 
ren gamedata_COC gamedata 
7z a xdOpenXRay.Mx_CoC86.7z .\* 
goto :END 

:MX_CoC64 
cd _build\bin\Win64\Mixed_COC 
copy *.dll ..\..\..\..\res\bins\ 
copy *.exe ..\..\..\..\res\bins\ 
cd ..\ 
copy Stalker-CoC.exe ..\..\..\res\ 
cd ..\..\..\ 
copy License.txt .\res\ 
copy README.md .\res\ 
cd res\ 
rd /s /q gamedata 
ren gamedata_COC gamedata 
7z a xdOpenXRay.Mx_CoC64.7z .\* 
goto :END 

:RX86 
cd _build\bin\Win32\Release 
copy *.dll ..\..\..\..\res\bins\ 
copy *.exe ..\..\..\..\res\bins\ 
cd ..\ 
copy OpenXRay.exe ..\..\..\res\ 
cd ..\..\..\ 
copy License.txt .\res\ 
copy README.md .\res\ 
cd res\ 
rd /s /q gamedata_COC 
7z a xdOpenXRay.Rx86.7z .\* 
goto :END 

:RX64 
cd _build\bin\Win64\Release 
copy *.dll ..\..\..\..\res\bins\ 
copy *.exe ..\..\..\..\res\bins\ 
cd ..\ 
copy OpenXRay.exe ..\..\..\res\ 
cd ..\..\..\ 
copy License.txt .\res\ 
copy README.md .\res\ 
cd res\ 
rd /s /q gamedata_COC 
7z a xdOpenXRay.Rx64.7z .\* 
goto :END 

:RX_CoC86 
cd _build\bin\Win32\Release_COC 
copy *.dll ..\..\..\..\res\bins\ 
copy *.exe ..\..\..\..\res\bins\ 
cd ..\ 
copy Stalker-CoC.exe ..\..\..\res\ 
cd ..\..\..\ 
copy License.txt .\res\ 
copy README.md .\res\ 
cd res\ 
rd /s /q gamedata 
ren gamedata_COC gamedata 
7z a xdOpenXRay.Rx_CoC86.7z .\* 
goto :END 

:RX_CoC64 
cd _build\bin\Win64\Release_COC 
copy *.dll ..\..\..\..\res\bins\ 
copy *.exe ..\..\..\..\res\bins\ 
cd ..\ 
copy Stalker-CoC.exe ..\..\..\res\ 
cd ..\..\..\ 
copy License.txt .\res\ 
copy README.md .\res\ 
cd res\ 
rd /s /q gamedata 
ren gamedata_COC gamedata 
7z a xdOpenXRay.Rx_CoC64.7z .\* 
goto :END 

:END 

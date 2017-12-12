md res\bins\ 

set CONFIGURATION=%1
set PLATFORM=%2

if %PLATFORM% == 'Debug' if %CONFIGURATION% == 'x86' goto :DX86 
if %PLATFORM% == 'Debug' if %CONFIGURATION% == 'x64' goto :DX64 
if %PLATFORM% == 'Release' if %CONFIGURATION% == 'x86' goto :RX86 
if %PLATFORM% == 'Release' if %CONFIGURATION% == 'x64' goto :RX64 

echo FAIL
goto :END 

:DX86 
cd bin\Win32\Debug 
copy *.dll ..\..\..\res\bins\ 
copy *.exe ..\..\..\res\bins\ 
cd ..\..\..\ 
dir 
copy License.txt .\res\ 
copy README.md .\res\ 
cd res\ 
7z a xdOpenXRay.Dx86.7z .\* 
goto :END 

:DX64 
cd bin\Win64\Debug 
copy *.dll ..\..\..\res\bins\ 
copy *.exe ..\..\..\res\bins\ 
cd ..\..\..\ 
dir 
copy License.txt .\res\ 
copy README.md .\res\ 
cd res\ 
7z a xdOpenXRay.Dx64.7z .\* 
goto :END 

:RX86 
cd bin\Win86\Release 
copy *.dll ..\..\..\res\bins\ 
copy *.exe ..\..\..\res\bins\ 
cd ..\..\..\ 
dir 
copy License.txt .\res\ 
copy README.md .\res\ 
cd res\ 
7z a xdOpenXRay.Rx86.7z .\* 
goto :END 

:RX64 
cd bin\Win64\Release 
copy *.dll ..\..\..\res\bins\ 
copy *.exe ..\..\..\res\bins\ 
cd ..\..\..\ 
dir 
copy License.txt .\res\ 
copy README.md .\res\ 
cd res\ 
7z a xdOpenXRay.Rx64.7z .\* 
goto :END 

:END 

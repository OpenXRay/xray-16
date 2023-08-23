@echo off

if [%1]==[] (
  echo Please, specify configuration
  EXIT /B
)

if [%2]==[] (
  echo Please, specify platform
  EXIT /B
)

set CONFIGURATION=%~1
set PLATFORM=%~2

if %PLATFORM%==x86 (
    set EDITION_NAME=%CONFIGURATION% 32-bit
) else if %PLATFORM%==x64 (
    set EDITION_NAME=%CONFIGURATION% 64-bit
) else (
    set EDITION_NAME=%CONFIGURATION% %PLATFORM%
)

if [%3]==[] (
    set PACKED_ARCHIVE_NAME=xpatch_openxray_plus.db
) else (
    set PACKED_ARCHIVE_NAME=%~3
)

rem Replace spaces with dots to avoid possible problems (e.g. with GitHub nighly builds uploading)
set EDITION_NAME=%EDITION_NAME: =.%

@echo on

md res\bin\
md res\bin\utils

rem Prepare files
copy "bin\%PLATFORM%\%CONFIGURATION%\*.dll" res\bin\
copy "bin\%PLATFORM%\%CONFIGURATION%\*.exe" res\bin\
copy "bin\%PLATFORM%\%CONFIGURATION%\*.pdb" res\bin\
copy "bin\%PLATFORM%\%CONFIGURATION%\utils\*" res\bin\utils\
copy License.txt res\
copy README.md res\
rem We don't need MFC stuff which Visual Studio automatically copies
del /q /f /s "res\bin\mfc*.dll"

cd res

rem Pack gamedata
bin\utils\xrCompress gamedata -ltx openxray_plus.ltx -filename %PACKED_ARCHIVE_NAME%
mkdir patches
move %PACKED_ARCHIVE_NAME% patches/%PACKED_ARCHIVE_NAME%

rem Make archives
7z a "OpenXRay.%EDITION_NAME%.7z" bin -xr!.* -xr!*.pdb -x!bin\utils -i!patches -i!fsgame.ltx -i!License.txt -i!README.md
7z a "Symbols.%EDITION_NAME%.7z" bin\*.pdb -i!License.txt -i!README.md -xr!.*
7z a "Utils.%EDITION_NAME%.7z" bin\utils\* -i!License.txt -i!README.md -xr!.*

cd ..

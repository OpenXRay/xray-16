
@echo off
rem ВАЖНО! Не должно быть каких-либо пробелов перед '=' или после пути!

rem Путь до папки репозитория, например: set PathToRepo=C:\Users\PC\Documents\Visual Studio 2017\Projects\OXR_CoC
set PathToRepo=C:\OXR_CoC
rem Путь до папки игры,        например: set PathToRepo=D:\games\CoC_target
set PathToGame=E:\Games\CoC_1.5_R7_Build

rem ______________________________________________________________________
rem ______________________________________________________________________
rem ______________________________________________________________________
rem ______________________________________________________________________
rem robocopy опасная утилита. Если что-то нужно поменять, то нужно быть осторожным
robocopy "%PathToRepo%\_build\bin\Win64\Release_COC" "%PathToGame%\bins" *.exe /MIR /Z /NJH /NJS
robocopy "%PathToRepo%\_build\bin\Win64\Release_COC" "%PathToGame%\bins" *.dll /MIR /Z /NJH /NJS
Pause
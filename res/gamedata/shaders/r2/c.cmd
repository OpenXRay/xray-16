@echo off
del test\*.ps
del test\*.vs
FOR %%a IN (*.ps) DO call p %%a
FOR %%b IN (*.vs) DO call v %%b

@echo off
del test\*.ps
del test\*.vs
FOR %%a IN (*.ps) DO call p1 %%a
FOR %%b IN (*.vs) DO call h2 %%b

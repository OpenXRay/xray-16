@echo off
rd /s/q HTML
rem "%ProgramFiles%\doxygen\bin\doxygen.exe" CompleteBugTrap.cfg
"%ProgramFiles%\doxygen\bin\doxygen.exe" BugTrap.cfg

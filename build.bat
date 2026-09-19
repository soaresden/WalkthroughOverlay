@echo off
rem Build with devkitPro (MSYS2). Requires DEVKITPRO to be set by the devkitPro installer.
set MSYS2_ARG_CONV_EXCL=*
make clean
make
pause

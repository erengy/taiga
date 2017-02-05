@echo off

echo Running vcvarsall.bat...
call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat"

cd src\curl\winbuild\

echo Building libcurl for Debug configuration...
nmake /f Makefile.vc mode=static RTLIBCFG=static VC=14 MACHINE=x86 DEBUG=yes
xcopy /s ..\builds\libcurl-vc14-x86-debug-static-ipv6-sspi-winssl\lib ..\..\..\lib\

echo Building libcurl for Release configuration...
nmake /f Makefile.vc mode=static RTLIBCFG=static VC=14 MACHINE=x86
xcopy /s ..\builds\libcurl-vc14-x86-release-static-ipv6-sspi-winssl\lib ..\..\..\lib\

echo Done!
pause

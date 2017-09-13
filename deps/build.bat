@echo off

set vc=%1
if "%vc%"=="15" (
  set vcvarsall="%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"
  goto :initialize_environment
)
if "%vc%"=="14" (
  set vcvarsall="%VS140COMNTOOLS%..\..\VC\vcvarsall.bat"
  goto :initialize_environment
)

set vswhere="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist %vswhere% (
  for /f "usebackq delims=" %%i in (`%vswhere% -latest -property installationPath`) do (
    set vcvarsall="%%i\VC\Auxiliary\Build\vcvars32.bat"
  )
  for /f "usebackq delims=." %%i in (`%vswhere% -latest -property installationVersion`) do (
    set vc=%%i
  )
  goto :initialize_environment
)

echo Error in script usage. The correct usage is: %~nx0 [version]
echo [version] can be "15" ^(VS2017^) or "14" ^(VS2015^)
echo Example: %~nx0 15
exit /B 1

:initialize_environment
echo Initializing environment...
call %vcvarsall% || (
  echo Please edit the build script according to your system configuration.
  exit /B 1
)

:build_libcurl
echo Configuring libcurl...

set currentdir=%cd%
cd /D %~dp0

call src\curl\buildconf.bat

if exist src\curl\builds (
  echo Clearing previous libcurl builds...
  rmdir /s /q src\curl\builds
)

cd src\curl\winbuild\

echo Building libcurl for Debug configuration...
nmake /f Makefile.vc mode=static RTLIBCFG=static VC=%vc% MACHINE=x86 DEBUG=yes
xcopy /s ..\builds\libcurl-vc%vc%-x86-debug-static-ipv6-sspi-winssl\lib ..\..\..\lib\

echo Building libcurl for Release configuration...
nmake /f Makefile.vc mode=static RTLIBCFG=static VC=%vc% MACHINE=x86
xcopy /s ..\builds\libcurl-vc%vc%-x86-release-static-ipv6-sspi-winssl\lib ..\..\..\lib\

cd /D %currentdir%

echo Done!
pause

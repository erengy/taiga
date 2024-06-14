@echo off

set machine=x86
set vc=16

:read_arguments
if "%1"=="--help" goto help
if "%1"=="--machine" set machine=%2
if "%1"=="--vc" set vc=%2
shift /1
shift /1
if not "%1"=="" goto read_arguments
goto locate_vs

:help
echo Usage: %~nx0 [--machine] [--vc]
echo:
echo   --machine    Target architecture, such as "x86", "x64" or "x64_arm64".
echo   --vc         Can be "16" ^(VS2019^). Overriden by vswhere, if available.
echo:
echo Example: %~nx0 --machine=x86 --vc=16
exit /B

:locate_vs
set vcvarsall="%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
set vswhere="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist %vswhere% (
  for /f "usebackq delims=" %%i in (`%vswhere% -latest -property installationPath`) do (
    set vcvarsall="%%i\VC\Auxiliary\Build\vcvarsall.bat"
  )
  for /f "usebackq delims=." %%i in (`%vswhere% -latest -property installationVersion`) do (
    set vc=%%i
  )
)

:initialize_environment
echo Initializing environment...
call %vcvarsall% %machine% || (
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
  for /d %%G in ("src\curl\builds\libcurl-vc%vc%-%machine%-*") do (rmdir /s /q "%%~G")
)

cd src\curl\winbuild\

echo Building libcurl for Debug^|%machine% configuration...
nmake /f Makefile.vc mode=static RTLIBCFG=static VC=%vc% MACHINE=%machine% DEBUG=yes
xcopy /s ..\builds\libcurl-vc%vc%-%machine%-debug-static-ipv6-sspi-schannel\lib ..\..\..\lib\%machine%\

echo Building libcurl for Release^|%machine% configuration...
nmake /f Makefile.vc mode=static RTLIBCFG=static VC=%vc% MACHINE=%machine%
xcopy /s ..\builds\libcurl-vc%vc%-%machine%-release-static-ipv6-sspi-schannel\lib ..\..\..\lib\%machine%\

cd /D %currentdir%

echo Done!
pause

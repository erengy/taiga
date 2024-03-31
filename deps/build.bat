@echo off

set vctarget=x86
set vc=16

:read_arguments
if "%1"=="--help" goto help
if "%1"=="--machine" set vctarget=%2
if "%1"=="--vc" set vc=%2
shift /1
shift /1
if not "%1"=="" goto read_arguments
goto locate_vs

:help
echo Usage: %~nx0 [--machine] [--vc]
echo:
echo   --machine    Target architecture, such as "x86", "x64" or "arm64".
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
set vchost=%PROCESSOR_ARCHITECTURE%
if /I %vchost%==amd64 set vchost=x64
if /I %vctarget%==amd64 set vctarget=x64
set vcarch=%vctarget%
if /I not %vchost%==%vcarch% set vcarch=%vchost%_%vcarch%
call %vcvarsall% %vcarch% || (
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
  for /d %%G in ("src\curl\builds\libcurl-vc%vc%-%vctarget%-*") do (rmdir /s /q "%%~G")
)

cd src\curl\winbuild\

echo Building libcurl for Debug^|%vctarget% configuration...
nmake /f Makefile.vc mode=static RTLIBCFG=static VC=%vc% MACHINE=%vctarget% DEBUG=yes
xcopy /s ..\builds\libcurl-vc%vc%-%vctarget%-debug-static-ipv6-sspi-schannel\lib ..\..\..\lib\%vctarget%\

echo Building libcurl for Release^|%vctarget% configuration...
nmake /f Makefile.vc mode=static RTLIBCFG=static VC=%vc% MACHINE=%vctarget%
xcopy /s ..\builds\libcurl-vc%vc%-%vctarget%-release-static-ipv6-sspi-schannel\lib ..\..\..\lib\%vctarget%\

cd /D %currentdir%

echo Done!
pause

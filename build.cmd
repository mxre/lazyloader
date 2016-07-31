@echo off
setlocal

set "BOOST_ROOT=%USERPROFILE%\Downloads\build\boost_1_59_0"

if "%VS140COMNTOOLS%"=="" (
	echo Could not detect VS14 common tools
	goto end
)
if NOT "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
	echo This is not a 64 bit architecture
	goto end
)

call "%VS140COMNTOOLS%\VCVarsQueryRegistry.bat" no32bit 64bit
if errorlevel 1 (
	echo Could not detect VS12 and SDK installation path
	goto end
)

set "WINSDK_LIB=%WindowsSdkDir%\lib\10.0.10586.0\ucrt\x64;%WindowsSdkDir%\lib\10.0.10586.0\um\x64;"
set "VCRT=%VCINSTALLDIR%\lib\amd64"
echo %VCINSTALLDIR%
set "INCLUDE=%VCINSTALLDIR%\include;%WindowsSdkDir%\Include\10.0.10586.0\shared"
set "INCLUDE=%INCLUDE%;%WindowsSdkDir%\Include\10.0.10586.0\um;
set "INCLUDE=%INCLUDE%;%WindowsSdkDir%\Include\10.0.10586.0\ucrt;
set "LIBPATH=%VCRT%;%WINSDK_LIB%"
set "LIB=%VCRT%;%WINSDK_LIB%"
::%LLVM_PATH%\msbuild-bin
set "PATH=%VCINSTALLDIR%\bin\amd64;%WindowsSdkDir%\bin\x64;%PATH%;%VS140COMNTOOLS%"

nmake /fnmake.mk %1%
if errorlevel 1 goto end
echo Done.
goto nopause
::cl /MD /O2 /Ob2 /D NDEBUG -DIL_STD /DWIN32 /D_WINDOWS /W3 /TP -Xclang -fcxx-exceptions -Xclang -std=c++11 -fms-compatibility-version=17.00.60610.1 -Xclang -dM -E - < NUL > C:\Users\Max\clang2.txt
:end
pause
:nopause
endlocal

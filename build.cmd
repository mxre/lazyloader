@echo off
setlocal

if "%VS140COMNTOOLS%"=="" (
	echo Could not detect VS14 common tools
	goto end
)
if NOT "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
	echo This is not a 64 bit architecture
	goto end
)

call "%VS140COMNTOOLS%\VCVarsQueryRegistry.bat" 64bit
if errorlevel 1 (
	echo Could not detect VS12 and SDK installation path
	goto end
)

for /F "tokens=1" %%i in ('reg query "HKLM\Software\Python\PythonCore"') do (
        set PythonRegistry=%%i
	)
)
for /F "tokens=1,2*" %%i in ('reg query "%PythonRegistry%\InstallPath" /ve') do (
        set PythonPath=%%k
	)
)
if "%PythonPath%"=="" (
	echo Could not detect Python3 installation
	goto end
)

set "PATH=%PythonPath%;%UniversalCRTSdkDir%bin\x64;%VSINSTALLDIR%VC\bin\amd64;%PATH%"
set "LIB=%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x64;%UniversalCRTSdkDir%lib\%UCRTVersion%\um\x64;%VSINSTALLDIR%VC\lib\amd64"
set "LIBPATH=%VSINSTALLDIR%VC\lib\amd64"
set "INCLUDE=%UniversalCRTSdkDir%Include\%UCRTVersion%\ucrt;%UniversalCRTSdkDir%Include\%UCRTVersion%\um;%UniversalCRTSdkDir%Include\%UCRTVersion%\shared;%VSINSTALLDIR%VC\include"

nmake /fnmake.mk %1%
if errorlevel 1 goto end

echo Done.

:end
@endlocal

#Requires -Version 1.0

# This file is part of lazycplex.
#
# Copyright 2016 Max Resch
#
# Permission is hereby granted, free of charge, to any
# person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal  in the
# Software without restriction, including without
# limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software
# is furnished to do so, subject to the following
# conditions:
#
# The above copyright notice and this permission notice
# shall be included in all copies or substantial portions
# of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
# KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
# THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
# PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
# CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# see http://www.opensource.org/licenses/MIT
#

if (-not (Test-Path env:VS140COMNTOOLS)) {
	Write-Output "Could not detect VS14 common tools"
	exit
}
if (-not ([Environment]::Is64BitOperatingSystem)) {
    Write-Output "This is not a 64 bit OS"
	exit
}

$oldpath = $env:PATH

# execute bath file and reexport environment
$tempfile = [io.Path]::GetTempFileName()
& "$ENV:Comspec" /c " ""$ENV:VS140COMNTOOLS\VCVarsQueryRegistry.bat"" 64bit && set" | Out-File -Filepath $tempfile
if (-not $?) {
	Write-Output "Could not detect VS14 and SDK installation path"
	exit
}
Get-Content $tempfile | foreach-object {
    if ($_ -match "^(.*?)=(.*)$") {
        set-item ENV:$($MATCHES[1]) $MATCHES[2]
    }
}
Remove-Item $tempfile

# detect python executable path
$PythonRegistry = (Get-ChildItem "HKLM:\SOFTWARE\Python\PythonCore")[0].Name

if (-not $?) {
	Write-Output "Could not detect python installation"
	exit
}

$PythonPath = (Get-ItemProperty "HKLM:\$PythonRegistry\InstallPath").'(default)'

$env:PATH    = "$PythonPath;${env:UniversalCRTSdkDir}bin\x64;${env:VSINSTALLDIR}VC\bin\amd64;${env:VSINSTALLDIR}VC\ClangC2\bin\amd64;$env:PATH"
$env:LIB     = "${env:UniversalCRTSdkDir}lib\$env:UCRTVersion\ucrt\x64;${env:UniversalCRTSdkDir}lib\$env:UCRTVersion\um\x64;${env:VSINSTALLDIR}VC\lib\amd64"
$env:LIBPATH = "${env:VSINSTALLDIR}VC\lib\amd64"
$env:INCLUDE = "${env:UniversalCRTSdkDir}Include\$env:UCRTVersion\ucrt;${env:UniversalCRTSdkDir}Include\$env:UCRTVersion\um;${env:UniversalCRTSdkDir}Include\$env:UCRTVersion\shared;${env:VSINSTALLDIR}VC\include"

New-Item -ItemType Directory generated
New-Item -ItemType Directory lib

& nmake /f nmake.mk

$env:PATH=$oldpath
@ECHO OFF &SETLOCAL

cd ..

ECHO Killing previous PIDs......
tasklist /v /fo csv | findstr /i "Build-Upload" > pid.csv
FOR /F "tokens=1,2* delims=,? " %%a in (pid.csv) do (
	Taskkill /PID %%b /F
)
del pid.csv

title=Build-Upload

ECHO Building source......
set LinuxPath=%~dp0
set LinuxPath=%LinuxPath:C:=/mnt/c%
set LinuxPath=%LinuxPath:\=/%
set LinuxPath=%LinuxPath%
bash -ilc "cd %LinuxPath%; ./flash.sh;"
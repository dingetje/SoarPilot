@echo off
for /F "usebackq" %%i in (`cd`) do SET CYGROOT=%%i
SET HOME=%CYGROOT%
SET HOMEPATH = \SP_Dev
SET PATH=%CYGROOT%\cygwin\bin;%PATH%
SET CYGROOT2=%CYGROOT%\cygwin
mount -f -u -b %CYGROOT2% /
mount -f -u -b %CYGROOT2%/bin /usr/bin
mount -f -u -b %CYGROOT2%/lib /usr/lib
for /F "delims=\:" %%i in ("%CYGROOT%") do SET CYGDRV=%%i
%CYGDRV%:
cd %CYGROOT2%
bash --login -i
umount -U

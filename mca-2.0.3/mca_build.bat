@echo off
cd ..\mxml-2.7\vcnet
call mxml1_build.bat
cd ..\..\mca-2.0.3\common
call common_build.bat
cd ..\mrapi
call mrapi_build.bat
cd ..\mcapi
call mcapi_build.bat
cd ..

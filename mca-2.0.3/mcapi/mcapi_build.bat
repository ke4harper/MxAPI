@echo off
cd src\mcapi_trans\mcapi_trans_sm
call mcapi_trans_sm_build.bat
cd ..\..\..
devenv /build debug /project mcapi mcapi.sln
devenv /build debug /project mcapi_abb mcapi.sln

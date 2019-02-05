@echo off
cd src\mrapi_impl
call mrapi_impl_build.bat
cd ..\..
devenv /build debug /project mrapi mrapi.sln
devenv /build debug /project mrapi_abb mrapi.sln

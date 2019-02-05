@echo off
cd sysvr4
call sysvr4_build.bat
cd ..
devenv /build debug /project mrapi_impl mrapi_impl.sln
devenv /build debug /project mrapi_impl_abb mrapi_impl.sln
devenv /build debug /project mrapi_implt mrapi_impl.sln
devenv /build debug /project mrapi_implt_abb mrapi_impl.sln
devenv /build debug /project mrapi_implp mrapi_impl.sln
devenv /build debug /project mrapi_implp_abb mrapi_impl.sln

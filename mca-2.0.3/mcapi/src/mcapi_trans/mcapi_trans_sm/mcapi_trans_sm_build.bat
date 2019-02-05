@echo off
cd mrapi
call transport_sm_mrapi_build.bat
cd ..
devenv /build debug /project mcapi_trans_sm mcapi_trans_sm.sln
devenv /build debug /project mcapi_trans_sm_abl mcapi_trans_sm.sln
devenv /build debug /project mcapi_trans_sm_abb mcapi_trans_sm.sln
devenv /build debug /project mcapi_trans_smt mcapi_trans_sm.sln
devenv /build debug /project mcapi_trans_smt_abl mcapi_trans_sm.sln
devenv /build debug /project mcapi_trans_smt_abb mcapi_trans_sm.sln
devenv /build debug /project mcapi_trans_smp mcapi_trans_sm.sln
devenv /build debug /project mcapi_trans_smp_abl mcapi_trans_sm.sln
devenv /build debug /project mcapi_trans_smp_abb mcapi_trans_sm.sln

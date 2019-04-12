/*
Copyright (c) 2012, ABB, Inc
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of ABB, Inc nor the names of its contributors may be
	  used to endorse or promote products derived from this software without
	  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

// mrapi_implp.cpp : Defines the entry point for the console application.
//

#if !(__unix__)
#include "stdafx.h"
#include <windows.h>
#else
#include <sys/wait.h>
#endif  // (__unix__)
#include <mrapi_sys_abb.h>

#define SYNC_BUFFERS 8
#define SYNC_THREADS 8
#define SYNC_PROCESSES 2

#include "Fragments/mrapi_implt_db_abb.cpp"

#if !(__unix__)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif  // (__unix__)
{
    mrapi_status_t status = 0;
    mrapi_mutex_hndl_t mutex_id = 0;
    int mutex_key = 0;
	mrapi_sem_hndl_t sem_id = 0;
	int sem_key = 0;
    mrapi_mutex_attributes_t mutex_attributes = { 0 };
	mrapi_sem_attributes_t sem_attributes = { 0 };
	mrapi_key_t lock_key = 0;
    uint32_t shmem_id = 0;
    int shmem_key = 0;
    mrapi_test_db_t* db = NULL;
#ifdef NOTUSED
    int magic = 1234;
#endif  // NOTUSED
    mrapi_shmem_attributes_t shmem_attributes = { 0 };

#if !(__unix__)
    STARTUPINFO si1 = { 0 };
    STARTUPINFO si2 = { 0 };
    PROCESS_INFORMATION pi1 = { 0 };
    PROCESS_INFORMATION pi2 = { 0 };
    _TCHAR szProgram[MAX_PATH] = TEXT("");
    _TCHAR szCmdLine1[MAX_PATH] = TEXT("");
    _TCHAR szCmdLine2[MAX_PATH] = TEXT("");
    LPTSTR szArg1 = _tcsdup(TEXT(" -d 1 -n 0 -s 4"));
    LPTSTR szArg2 = _tcsdup(TEXT(" -d 1 -n 10 -s 4"));

    si1.cb = sizeof(si1);
    si2.cb = sizeof(si2);
#else
    int i = 0;
	int pid = 0;
	int wstatus = 0;
	int argv0size = strlen(argv[0]);
	char szCmdLine1[][10] = { "-d","1","-n","0","-s","4" };
	char szCmdLine2[][10] = { "-d","1","-n","10","-s","4" };
#endif  // (__unix__)

	mca_set_debug_level(0);

	// Initialize runtime for process tests
	assert(mrapi_impl_initialize(1,1,&status));

    assert(sys_file_key(NULL,'a',&mutex_key));
    mrapi_impl_mutex_init_attributes(&mutex_attributes);
    assert(mrapi_impl_mutex_create(&mutex_id,mutex_key,&mutex_attributes,&status));
	sem_key = mutex_key + 10;
	mrapi_impl_sem_init_attributes(&sem_attributes);
	assert(mrapi_impl_sem_create(&sem_id, sem_key, &sem_attributes, 1, &status));

    assert(sys_file_key(NULL,'b',&shmem_key));
    mrapi_impl_shmem_init_attributes(&shmem_attributes);
    status = MRAPI_SUCCESS;
    mrapi_impl_shmem_create(&shmem_id,shmem_key,sizeof(mrapi_test_db_t),&shmem_attributes,&status);
    assert(MRAPI_SUCCESS == status);
    db = (mrapi_test_db_t*)mrapi_impl_shmem_attach(shmem_id); // Hold a reference count
    assert(NULL != db);

#if !(__unix__)
#if (__MINGW32__)
#if _DEBUG
    _tcscpy(szProgram,TEXT("Debug\\mrapi_implt_abb.exe"));
#else
    _tcscpy(szProgram,TEXT("Release\\mrapi_implt_abb.exe"));
#endif  // !DEBUG
    _tcscpy(szCmdLine1,szProgram);
    _tcscpy(szCmdLine2,szProgram);
    _tcscat(szCmdLine1,szArg1);
    _tcscat(szCmdLine2,szArg2);
#else
#if _DEBUG
    _tcscpy_s(szProgram,sizeof(szProgram)/sizeof(_TCHAR),TEXT("Debug\\mrapi_implt_abb.exe"));
#else
    _tcscpy_s(szProgram,sizeof(szProgram)/sizeof(_TCHAR),TEXT("Release\\mrapi_implt_abb.exe"));
#endif  // !DEBUG
    _tcscpy_s(szCmdLine1,sizeof(szCmdLine1)/sizeof(_TCHAR),szProgram);
    _tcscpy_s(szCmdLine2,sizeof(szCmdLine2)/sizeof(_TCHAR),szProgram);
    _tcscat_s(szCmdLine1,sizeof(szCmdLine1)/sizeof(_TCHAR),szArg1);
    _tcscat_s(szCmdLine2,sizeof(szCmdLine2)/sizeof(_TCHAR),szArg2);
#endif  // !(__MINGW32__)
    if(!CreateProcess(szProgram, szCmdLine1,
        NULL, NULL, FALSE, 0, NULL, NULL, &si1, &pi1)) {
      _tprintf(TEXT("Failed to start %s\n"),szCmdLine1);
    }
    if(!CreateProcess(szProgram, szCmdLine2,
        NULL, NULL, FALSE, 0, NULL, NULL, &si2, &pi2)) {
      _tprintf(TEXT("Failed to start %s\n"),szCmdLine2);
    }
#else
	pid = fork();
	if(0 == pid) {
		// child process
		argv[0][argv0size-5] = 't';
		execl(argv[0],argv[0],
              szCmdLine1[0],szCmdLine1[1],szCmdLine1[2],
              szCmdLine1[3],szCmdLine1[4],szCmdLine1[5],
              (char*)0);
	}
	else if(0 > pid) {
		printf("mrapi_implt_abb fork failed\n");
		exit(1);
	}
	pid = fork();
	if(0 == pid) {
		// child process
		argv[0][argv0size-5] = 't';
		execl(argv[0],argv[0],
              szCmdLine2[0],szCmdLine2[1],szCmdLine2[2],
              szCmdLine2[3],szCmdLine2[4],szCmdLine2[5],
              (char*)0);
	}
	else if(0 > pid) {
		printf("mrapi_implt_abb fork failed\n");
		exit(1);
	}
#endif  // (__unix__)

#if !(__unix__)
    // Wait until second child process exits.
    WaitForSingleObject(pi2.hProcess, INFINITE);
    CloseHandle( pi2.hProcess );
    CloseHandle( pi2.hThread );

    // Wait until first child process exits.
    WaitForSingleObject(pi1.hProcess, INFINITE);

    // Close process and thread handles.
    CloseHandle( pi1.hProcess );
    CloseHandle( pi1.hThread );

	// Release string arguments
	free((void*)szArg1);
	free((void*)szArg2);
#else
    for(i = 2; 0 < i;) {
        pid_t pid = wait(&wstatus);
        if(0 < pid) {
            if(WIFEXITED(wstatus)) {
                //printf("mrapi_implp_abb child %d exit %d\n", pid,WEXITSTATUS(wstatus));
                i--;
            }
            if(WIFSIGNALED(wstatus)) {
                printf("mrapi_implp_abb child %d signal %d\n", pid,WTERMSIG(wstatus));
                //i--;
            }
        }
    }
#endif  // (__unix__)

    assert(mrapi_impl_mutex_lock(mutex_id,&lock_key,MRAPI_TIMEOUT_INFINITE,&status));
    assert(mrapi_impl_mutex_delete(mutex_id));
	assert(mrapi_impl_sem_lock(sem_id, 1, MRAPI_TIMEOUT_INFINITE, &status));
	assert(mrapi_impl_sem_delete(sem_id));
	assert(mrapi_impl_shmem_detach(shmem_id)); // release reference count
    assert(mrapi_impl_shmem_delete(shmem_id));

    assert(mrapi_impl_finalize());

    return 0;
}


// mrapi_implp.cpp : Defines the entry point for the console application.
//

#if !(__unix__)
#include "stdafx.h"
#include <windows.h>
#else
#include <sys/wait.h>
#endif  // (__unix__)
#include <mrapi_sys.h>

#define SYNC_BUFFERS 8
#define SYNC_THREADS 8
#define SYNC_PROCESSES 2

#include "Fragments/mrapi_implt_db.cpp"

#if !(__unix__)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif  // (__unix__)
{
    mrapi_status_t status = 0;
    mrapi_mutex_id_t mutex_id = 0;
    mrapi_mutex_hndl_t mutex = 0;
    mrapi_mutex_attributes_t mutex_attributes = { 0 };
    mrapi_key_t lock_key = 0;
    mrapi_shmem_id_t shmem_id = 0;
    mrapi_shmem_hndl_t shmem = 0;
    mrapi_shmem_attributes_t shmem_attributes = { 0 };
    void* addr = NULL;

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
	int pid = 0;
	int wstatus = 0;
	int argv0size = strlen(argv[0]);
	char szCmdLine1[][10] = { "-d","1","-n","0","-s","1" };
	char szCmdLine2[][10] = { "-d","1","-n","10","-s","1" };
#endif  // (__unix__)

    printf("stress 1 1 start\n");

	// Initialize runtime for process tests
	assert(mrapi_impl_initialize(1,1,&status));

    mutex_id = mca_Crc32_ComputeBuf(0,"mrapi_implt_mutex_stress",24);
    mrapi_impl_mutex_init_attributes(&mutex_attributes);
    assert(mrapi_impl_mutex_create(&mutex,mutex_id,&mutex_attributes,&status));

    shmem_id = mca_Crc32_ComputeBuf(0,"mrapi_implt_shmem_stress",24);
    mrapi_impl_shmem_init_attributes(&shmem_attributes);
    status = MRAPI_SUCCESS;
    mrapi_impl_shmem_create(&shmem,shmem_id,sizeof(mrapi_test_db_t),&shmem_attributes,&status);
    assert(MRAPI_SUCCESS == status);
    addr = mrapi_impl_shmem_attach(shmem); // Hold a reference count
    assert(NULL != addr);

#if !(__unix__)
#if (__MINGW32__)
#if _DEBUG
    _tcscpy(szProgram,TEXT("Debug\\mrapi_implt.exe"));
#else
    _tcscpy(szProgram,TEXT("Release\\mrapi_implt.exe"));
#endif  // !DEBUG
    _tcscpy(szCmdLine1,szProgram);
    _tcscpy(szCmdLine2,szProgram);
    _tcscat(szCmdLine1,szArg1);
    _tcscat(szCmdLine2,szArg2);
#else
#if _DEBUG
    _tcscpy_s(szProgram,sizeof(szProgram)/sizeof(_TCHAR),TEXT("Debug\\mrapi_implt.exe"));
#else
    _tcscpy_s(szProgram,sizeof(szProgram)/sizeof(_TCHAR),TEXT("Release\\mrapi_implt.exe"));
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
		argv[0][argv0size-1] = 't';
		execl(argv[0],argv[0],
              szCmdLine1[0],szCmdLine1[1],szCmdLine1[2],
              szCmdLine1[3],szCmdLine1[4],szCmdLine1[5],
              (char*)0);
	}
	else if(0 > pid) {
		printf("mrapi_implt fork failed\n");
		exit(1);
	}
	pid = fork();
	if(0 == pid) {
		// child process
		argv[0][argv0size-1] = 't';
		execl(argv[0],argv[0],
              szCmdLine2[0],szCmdLine2[1],szCmdLine2[2],
              szCmdLine2[3],szCmdLine2[4],szCmdLine2[5],
              (char*)0);
	}
	else if(0 > pid) {
		printf("mrapi_implt fork failed\n");
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
	wait(&wstatus);
#endif  // (__unix__)

    assert(mrapi_impl_shmem_detach(shmem)); // release reference count
    assert(mrapi_impl_shmem_delete(shmem));
    assert(mrapi_impl_mutex_lock(mutex,&lock_key,MRAPI_TIMEOUT_INFINITE,&status));
    assert(mrapi_impl_mutex_delete(mutex));

    assert(mrapi_impl_finalize());

    return 0;
}


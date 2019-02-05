// mcapi_trans_smp_abb.cpp : Defines the entry point for the console application.
//

#if !(__unix__)
#include "stdafx.h"
#include <windows.h>
#else
#include <sys/wait.h>
#endif  // (__unix__)
#include <mrapi_abb.h>
#include <mrapi_sys_abb.h>
#include <mcapi_trans_abl.h>
#include <transport_sm_abl.h>

#if !(__unix__)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif  // (__unix__)
{
    mcapi_status_t status = 0;
	mcapi_domain_t domain = 0;
	mcapi_node_t node = 0;
    mcapi_node_attributes_t node_attrs = { { { 0 } } };

#if !(__unix__)
    STARTUPINFO si1 = { 0 };
    STARTUPINFO si2 = { 0 };
    PROCESS_INFORMATION pi1 = { 0 };
    PROCESS_INFORMATION pi2 = { 0 };
    _TCHAR szProgram[MAX_PATH] = TEXT("");
    _TCHAR szCmdLine1[MAX_PATH] = TEXT("");
    _TCHAR szCmdLine2[MAX_PATH] = TEXT("");
    _TCHAR szMode[6] = TEXT("0x7");
    LPTSTR szArg1 = _tcsdup(TEXT(" -d 1 -n 1 -m "));
    LPTSTR szArg2 = _tcsdup(TEXT(" -d 1 -n 11 -m "));

    si1.cb = sizeof(si1);
    si2.cb = sizeof(si2);
#else
    int i = 0;
	int pid = 0;
	int wstatus = 0;
	int argv0size = strlen(argv[0]);
	char szCmdLine1[][10] = { "-d","1","-n","1","-m" };
	char szCmdLine2[][10] = { "-d","1","-n","11","-m" };
    char szMode[6] = "0x7";
#endif  // (__unix__)

    int start = 0;
#if !(__unix__)
    _TCHAR opt = 0;
    _TCHAR* param = NULL;
    _TCHAR* xml = NULL;
#else
    char opt = 0;
    char* param = NULL;
    char* xml = NULL;
#endif  // (__unix__)

    xml = argv[1];
    start = 1;
    while(argc > start) {
        if(sys_os_getopt(argc, argv, &start, &opt, &param)) {
#if !(__unix__)
            switch(towlower(opt)) {
            case L'm': // -m <mode>
                wcscpy_s(szMode,sizeof(szMode)/sizeof(_TCHAR),param);
                break;
            }
#else
            switch(tolower(opt)) {
            case 'm': // -m <mode>
                strcpy(szMode,param);\
                break;
            }
#endif  // (__unix__)
        }
    }

    printf("stress 1 1 start\n");

	mca_set_debug_level(0);

	// Initialize runtime for remainder of tests
	domain = 1;
	node = 1;
    assert(mcapi_trans_node_init_attributes(&node_attrs,&status));
	assert(mcapi_trans_initialize(domain,node,&node_attrs,MCAPI_TRUE));

#if !(__unix__)
#if (__MINGW32__)
#if _DEBUG
    _tcscpy(szProgram,TEXT("Debug\\mcapi_trans smt_abl.exe "));
#else
    _tcscpy(szProgram,TEXT("Release\\mcapi_trans_smt_abl.exe "));
#endif  // !DEBUG
    _tcscpy(szCmdLine1,szProgram);
    _tcscpy(szCmdLine2,szProgram);
    _tcscat(szCmdLine1,xml);
    _tcscat(szCmdLine2,xml);
    _tcscat(szCmdLine1,szArg1);
    _tcscat(szCmdLine2,szArg2);
#else
#if _DEBUG
    _tcscpy_s(szProgram,sizeof(szProgram)/sizeof(_TCHAR),TEXT("Debug\\mcapi_trans_smt_abl.exe "));
#else
    _tcscpy_s(szProgram,sizeof(szProgram)/sizeof(_TCHAR),TEXT("Release\\mcapi_trans_smt_abl.exe "));
#endif  // !DEBUG
    _tcscpy_s(szCmdLine1,sizeof(szCmdLine1)/sizeof(_TCHAR),szProgram);
    _tcscpy_s(szCmdLine2,sizeof(szCmdLine2)/sizeof(_TCHAR),szProgram);
    _tcscat_s(szCmdLine1,sizeof(szCmdLine1)/sizeof(_TCHAR),xml);
    _tcscat_s(szCmdLine2,sizeof(szCmdLine2)/sizeof(_TCHAR),xml);
    _tcscat_s(szCmdLine1,sizeof(szCmdLine1)/sizeof(_TCHAR),szArg1);
    _tcscat_s(szCmdLine2,sizeof(szCmdLine2)/sizeof(_TCHAR),szArg2);
    _tcscat_s(szCmdLine1,sizeof(szCmdLine1)/sizeof(_TCHAR),szMode);
    _tcscat_s(szCmdLine2,sizeof(szCmdLine2)/sizeof(_TCHAR),szMode);
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
		execl(argv[0],argv[0],xml,
              szCmdLine1[0],szCmdLine1[1],szCmdLine1[2],
              szCmdLine1[3],szCmdLine1[4],
              szMode,
              (char*)0);
	}
	else if(0 > pid) {
		printf("mrapi_implt fork failed\n");
		exit(1);
	}
	pid = fork();
	if(0 == pid) {
		// child process
		argv[0][argv0size-5] = 't';
		execl(argv[0],argv[0],xml,
              szCmdLine2[0],szCmdLine2[1],szCmdLine2[2],
              szCmdLine2[3],szCmdLine2[4],
              szMode,
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

	assert(mcapi_trans_finalize());

    return 0;
}

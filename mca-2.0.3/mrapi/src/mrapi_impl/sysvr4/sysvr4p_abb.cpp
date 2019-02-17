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

// sysvr4p_abb.cpp : Defines the entry point for the console application.
//

#include "Fragments/sysvr4p_inc_abb.cpp"
#include "Fragments/sysvr4p_type.cpp"

#if !(__unix__)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif  // (__unix__)
{
#if !(__unix__)
    STARTUPINFO si1 = { 0 };
    STARTUPINFO si2 = { 0 };
    PROCESS_INFORMATION pi1 = { 0 };
    PROCESS_INFORMATION pi2 = { 0 };
    _TCHAR szProgram[MAX_PATH] = TEXT("");
    _TCHAR szCmdLine[MAX_PATH] = TEXT("");
    _TCHAR szArg[10] = TEXT(" -p 0");

    si1.cb = sizeof(si1);
    si2.cb = sizeof(si2);
#else
	int pid = 0;
	int wstatus = 0;
	int argv0size = strlen(argv[0]);
	char szCmdLine[][10] = { "-p","0" };
#endif  // (__unix__)
    int i = 0;
    int lsem = 0;
    int sem_key = 0;
    uint32_t dbid = 0;
    uint32_t xchgid = 0;
    int shmem_db = 0;
    int shmem_xchg = 0;
    shmem_db_t* db = NULL;
    shmem_xchg_t* xchg = NULL;

    mca_set_debug_level(0);

    assert(sys_file_key(NULL,'a',&sem_key));
    assert(sys_sem_create(sem_key,1,&lsem));
    assert(0 < (int)lsem);

    assert(sys_file_key(NULL,'b',&shmem_db));
    dbid = sys_shmem_create(shmem_db,sizeof(shmem_db_t));
    assert(0 < (int)dbid);
    db = (shmem_db_t*)sys_shmem_attach(dbid);
    assert(NULL != db);
    memset(db,0,sizeof(shmem_db_t));

    assert(sys_file_key(NULL,'c',&shmem_xchg));
    xchgid = sys_shmem_create(shmem_xchg,sizeof(shmem_xchg_t));
    assert(0 < (int)xchgid);
    xchg = (shmem_xchg_t*)sys_shmem_attach(xchgid);
    assert(NULL != xchg);
    memset(xchg,0,sizeof(shmem_xchg_t));

#if !(__unix__)
#if (__MINGW32__)
#if _DEBUG
    _tcscpy(szProgram,TEXT("Debug\\sysvr4t_abb.exe"));
#else
    _tcscpy(szProgram,TEXT("Release\\sysvr4t_abb.exe"));
#endif  // !DEBUG
    _tcscpy(szCmdLine,szProgram);
    _tcscat(szCmdLine,szArg);
#else
#if _DEBUG
    _tcscpy_s(szProgram,sizeof(szProgram)/sizeof(_TCHAR),TEXT("Debug\\sysvr4t_abb.exe"));
#else
    _tcscpy_s(szProgram,sizeof(szProgram)/sizeof(_TCHAR),TEXT("Release\\sysvr4t_abb.exe"));
#endif  // !DEBUG
    _tcscpy_s(szCmdLine,sizeof(szCmdLine)/sizeof(_TCHAR),szProgram);
    _tcscat_s(szCmdLine,sizeof(szCmdLine)/sizeof(_TCHAR),szArg);
#endif  // !(__MINGW32__)
    szCmdLine[_tcslen(szCmdLine)-1] = TEXT('1');
    if(!CreateProcess(szProgram, szCmdLine,
        NULL, NULL, FALSE, 0, NULL, NULL, &si1, &pi1)) {
      _tprintf(TEXT("Failed to start %s\n"),szCmdLine);
    }
    szCmdLine[_tcslen(szCmdLine)-1] = TEXT('2');
    if(!CreateProcess(szProgram, szCmdLine,
        NULL, NULL, FALSE, 0, NULL, NULL, &si2, &pi2)) {
      _tprintf(TEXT("Failed to start %s\n"),szCmdLine);
    }
#else
	pid = fork();
	if(0 == pid) {
		// child process
		argv[0][argv0size-5] = 't';
        szCmdLine[1][strlen(szCmdLine[1]-1)] = '1';
		execl(argv[0],argv[0], szCmdLine[0], szCmdLine[1], (char*)0);
	}
	else if(0 > pid) {
		printf("sysvr4t fork failed\n");
		_exit(1);
	}
	pid = fork();
	if(0 == pid) {
		// child process
		argv[0][argv0size-5] = 't';
        szCmdLine[1][strlen(szCmdLine[1]-1)] = '2';
		execl(argv[0],argv[0], szCmdLine[0], szCmdLine[1], (char*)0);
	}
	else if(0 > pid) {
		printf("sysvr4t fork failed\n");
		_exit(1);
	}
#endif  // (__unix__)

    // Spin waiting for child process handles
    for(i = 0; i < 2; i++) {
        while(0 == xchg->proc[i]) {
            sys_os_yield();
        }
        // Duplicate shared memory handle
        assert(sys_shmem_duplicate(dbid,xchg->proc[i],&xchg->shmid[i]));
    }

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
#else
    for(i = 2; 0 < i;) {
        (void)wait(&wstatus);
        if(WIFEXITED(wstatus)) {
            i--;
        }
    }
#endif  // (__unix__)

    assert(sys_sem_delete(lsem));
    assert(sys_shmem_detach(db));
    assert(sys_shmem_delete(dbid));
    assert(sys_shmem_detach(xchg));
    assert(sys_shmem_delete(xchgid));

    return 0;
}

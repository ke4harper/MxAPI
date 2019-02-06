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
	* Neither the name of the <organization> nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

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

/***************************************************************************
  Function: sys_shmem_detach

  Description: frees the shared memory for the given address.

  Parameters: shm_address - shared memory address

  Returns: boolean indicating success or failure

  ***************************************************************************/
  mrapi_boolean_t sys_shmem_detach(void *shm_address){

    /* The database should already be locked ! */

    mrapi_dprintf(1,"sys_shmem_detach addr=%p",shm_address);
    /* detach the shared memory segment */
#if (__unix__)
    int rc = shmdt(shm_address);
    if (rc==-1) {
      printf("ERROR: mrapi: sys_shmem_detach addr=%p shmdt() failed errno:%s\n",shm_address,strerror(errno));
      return MRAPI_FALSE;
    }
#else
    if(!UnmapViewOfFile(shm_address)) {
#if (__MINGW32__)
      printf("ERROR: mrapi: sys_shmem_detach failed errno:%s\n",strerror(EINVAL));
#else
      char buf[80];
	  strerror_s(buf,80,EINVAL);
      printf("ERROR: mrapi: sys_shmem_detach failed errno:%s\n",buf);
#endif  /* (__MINGW32__) */
      return MRAPI_FALSE;
    }
#endif  /* !(__unix__) */
    return MRAPI_TRUE;
  }

  /***************************************************************************
    Function: sys_sem_release

    Description: releases the shared memory resources for the given id,
                 complementing a corresponding get

    Parameters:

    Returns: boolean indicating success or failure

   ***************************************************************************/
  mrapi_boolean_t sys_shmem_release(uint32_t shmid){
    mrapi_boolean_t rc = MRAPI_TRUE;
    /* Unix does not have separate resources associated with a get */
#if !(__unix__)
    CloseHandle((HANDLE)shmid);
#endif  /* !(__unix__) */
    mrapi_dprintf(1,"sys_shmem_release deallocating shared memory:%d",shmid);
    return rc;
  }

  /***************************************************************************
  Function: sys_shmem_delete

  Description: frees the shared memory for the given id.

  Parameters: shmid - shared memory id
              id - shared memory id

  Returns:

  ***************************************************************************/
  mrapi_boolean_t sys_shmem_delete(uint32_t shmid){
    mrapi_boolean_t rc = MRAPI_TRUE;
    /* The database should already be locked ! */

    mrapi_dprintf(1,"sys_shmem_delete removing id=%d",shmid);

    /* delete the shared memory id */
#if (__unix__)
    rc = shmctl(shmid, IPC_RMID, 0);
    if (rc == -1)  {
      printf("ERROR: mrapi: sys_shmem_delete shmctl() failed errno: %s\n",strerror(errno));
      return MRAPI_FALSE;
    }
#else
	CloseHandle((HANDLE)shmid);
#endif  /* !(__unix__) */
    return MRAPI_TRUE;
  }

  /***************************************************************************
  Function: sys_shmem_get

  Description: Returns shared memory segment id for a given key.

  Parameters: shmemkey - shared memory key
              size - the desired size (in bytes)

  Returns: shmid: the shared memory id

  ***************************************************************************/
  uint32_t sys_shmem_get(uint32_t shmemkey,int size) {

    /* the database should already be locked */
#if (__unix__)
    uint32_t shmid = -1;
    while(-1 == shmid) {
      shmid = shmget((key_t)shmemkey, size, 0666);
      if(EAGAIN == errno) {
        sched_yield();
      }
      else {
         break;
      }
    }
    mrapi_dprintf(1,"sys_shmem_get: shmem: %d size: %d shmid: %d",shmemkey,size,shmid);
#else
    /* Use ASCII representation of key for file mapping object name */
    uint32_t shmid = 0;
#if (__MINGW32__)
    char name[80] = "";
	sprintf(name,"Local\\mca_%u",shmemkey);
#else
    int done = 0;
    char buf[80];
	wchar_t name[80] = L"";
	swprintf(name,80,L"Local\\mca_%u",shmemkey);
#endif  /* !(__MINGW32__) */
    while(!done) {
      shmid = (uint32_t)OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,name);
      if(0 == shmid) {
        DWORD err = GetLastError();
        switch(err) {
        case ERROR_FILE_NOT_FOUND:
            shmid = -1;
            errno = EINVAL;
            done = 1;
            break;

        }
      }
      else {
        done = 1;
        break;
      }
    }
#endif  /* !(__unix__) */

#if (__unix__||__MINGW32__)
    mrapi_dprintf(1,"sys_shmem_get errno:%s",strerror(errno));
#else
    strerror_s(buf,80,errno);
    mrapi_dprintf(1,"sys_shmem_get errno:%s",buf);
#endif  /* !(__unix__||__MINGW32__) */

    return shmid;
  }

  /***************************************************************************
    Function: sys_shmem_duplicate

    Description: Returns duplicated shared memory id

    Parameters: shmid - shared memory id to be duplicated
                tprocid - target process id
                tshmid - returned duplicate for target

    Returns: boolean indicating success or failure

  ***************************************************************************/
  mrapi_boolean_t sys_shmem_duplicate(uint32_t shmid,int tprocid,uint32_t* tshmid) {
    mrapi_boolean_t rc = MRAPI_TRUE;
#if (__unix__)
    /* Global key used across processes, no duplication necessary */
    *tshmid = shmid;
#else
    HANDLE proc = 0;
    mrapi_dprintf(1,"sys_shmem_duplicate shmid %d tprocid %d",shmid,tprocid);

    /* Open target process */
    proc = OpenProcess(PROCESS_DUP_HANDLE,FALSE,(DWORD)tprocid);
    if(0 == proc) {
      DWORD err = GetLastError();
      switch(err) {
      case 0:
          break;
      }
      rc = MRAPI_FALSE;
    }

    /* Inheritable handle with same access rights as source */
    if(rc && 0 == DuplicateHandle(GetCurrentProcess(),(HANDLE)shmid,proc,(HANDLE*)tshmid,0,TRUE,DUPLICATE_SAME_ACCESS)) {
      DWORD err = GetLastError();
      printf("sys_shmem_duplicate err %d\n",err);
      assert(FALSE);
      switch(err) {
      case 0:
          break;
      }
      rc = MRAPI_FALSE;
    }
#endif  /* !(__unix__) */

    return rc;
  }

  /***************************************************************************
  Function: sys_shmem_create

  Description: Returns shared memory segment id for a given key.

  Parameters: shmemkey - shared memory key
              size - the desired size (in bytes)

  Returns: shmid: the shared memory id

  ***************************************************************************/
  uint32_t sys_shmem_create(uint32_t shmemkey,int size){

    /* the database should already be locked */
#if (__unix__)
	errno = 0;
    uint32_t shmid = shmget((key_t)shmemkey, size, 0666 | IPC_CREAT | IPC_EXCL);
#else
    /* Use ASCII representation of key for file mapping object name */
    uint32_t shmid = 0;
#if (__MINGW32__)
	char name[80] = "";
	sprintf(name,"Local\\mca_%u",shmemkey);
#else
    char buf[80];
	wchar_t name[80] = L"";
	swprintf(name,80,L"Local\\mca_%u",shmemkey);
#endif  /* (__MINGW32__) */
    shmid = (uint32_t)CreateFileMapping((HANDLE)INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,size,name);
	if((uint32_t)NULL == shmid) {
      shmid = -1;
      errno = EINVAL;
    }
    else {
      errno = 0;
    }
#endif  /* !(__unix__) */

#if (__unix__||__MINGW32__)
    mrapi_dprintf(1,"sys_shmem_create errno:%s",strerror(errno));
#else
    strerror_s(buf,80,errno);
    mrapi_dprintf(1,"sys_shmem_create errno:%s",buf);
#endif  /* !(__unix__||__MINGW32__) */

    return shmid;
  }

  /***************************************************************************
  Function: sys_shmem_attach

  Description: attaches the process to the shared memory for the given id.

  Parameters: shmid - shared memory id

  Returns: NULL->FAIL, otherwise address of shared memory segment

  ***************************************************************************/
  void* sys_shmem_attach(int shmid){

    /* the database should already be locked */
#if (__unix__)
    struct shmid_ds dsbuf;
    void* addr = shmat(shmid, 0, 0);
    if ((long)addr == (-1)) {
      mrapi_dprintf(1,"Warning: mrapi sys_shmem_attach PPID %d: shmat %u failed! errno %d '%s'",
                    getpid(), (unsigned)shmid, errno, strerror(errno));
      return NULL;
    }

    if (shmctl(shmid, IPC_STAT, &dsbuf)) {
      mrapi_dprintf(1,"Warning: mrapi sys_shmem_attach PPID %d: shmctl %u failed! errno %d '%s'",
                    getpid(), (unsigned)shmid, errno, strerror(errno));
      shmdt(addr);
      return NULL;
    }

#if 0
  /* if we are the first to attach, then initialize the segment to 0 */
    if (dsbuf.shm_nattch == 1) {
      memset(addr,0,dsbuf.shm_segsz);
    }
#endif
    mrapi_dprintf(1,"sys_shmem_attach: shmem:%u nattch:%d size:%d", shmid, (int)dsbuf.shm_nattch,(int)dsbuf.shm_segsz);
#else
    void* addr = MapViewOfFile((HANDLE)shmid,FILE_MAP_WRITE,0,0,0);
    if(NULL == addr) {
      DWORD err = GetLastError();
      mrapi_dprintf(1,"Warning: mrapi sys_shmem_attach PPID %d: MapViewOfFile failed! err %d",
          GetCurrentProcessId(),err);
      CloseHandle((HANDLE)shmid);
    }
#endif  /* !(__unix__) */
    return addr;
  }

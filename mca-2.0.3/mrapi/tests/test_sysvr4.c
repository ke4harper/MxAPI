/*
Copyright (c) 2010, The Multicore Association
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

(1) Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

(2) Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

(3) Neither the name of the Multicore Association nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Port to Windows: #if !(__unix||__MINGW32__), etc.

*/
#if (__unix)
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#else
#include <windows.h>
typedef int key_t;
#endif  /* !(__unix) */
#include <sys/types.h>   

#include <errno.h>
//#include <stdlib.h>
#include <stdio.h>    
#include <string.h>

void* addr;
int shmid;
int semid; 

#if (__unix)
#define TRUE 0
#define FALSE 1
#endif  /* (__unix) */

#if !(__unix)
  typedef struct {
    int num_locks;
	HANDLE* sem;
  } sem_set_t;

/***************************************************************************
  Function: sys_release_sem_set
  
  Description: 
  
  Parameters: 
  
  Returns: 0 on success, -1 on failure
  
 ***************************************************************************/
int sys_release_sem_set(sem_set_t* ss) {
  int rc = 0;
  if(NULL != ss) {
    if(NULL != ss->sem) {
      int i = 0;
	  for(i = 0; i < ss->num_locks; i++) {
	    if(NULL != ss->sem[i]) {
		  CloseHandle(ss->sem[i]);
		  ss->sem[i] = NULL;
		}
	  }
	  free((void*)ss->sem);
	}
	free((void*)ss);
  }
  return rc;
}

/***************************************************************************
  Function: sys_alloc_sem_set
  
  Description: 
  
  Parameters: 
  
  Returns: pointer to semaphore set or NULL
  
 ***************************************************************************/
sem_set_t* sys_alloc_sem_set(int num_locks) {
  sem_set_t* ss = NULL;
  ss = (sem_set_t*)malloc(sizeof(sem_set_t));
  if(NULL != ss) {
    memset(ss,0,sizeof(sem_set_t));
    ss->num_locks = num_locks;
	if(0 < num_locks) {
      ss->sem = (HANDLE*)malloc(num_locks*sizeof(HANDLE));
      if(NULL != ss->sem) {
        memset(ss->sem, 0, num_locks*sizeof(HANDLE));
	  }
	}
	return ss;
  }
  sys_release_sem_set(ss);
  return NULL;
}
#endif  /* !(__unix) */

int create_sem() {
  int rc = TRUE; 
  key_t key = 0xbeefdead;
  int num_locks = 1;
#if (__unix)
  struct sembuf sbuf;
#else
  int i = 0;
#if (__MINGW32__)
  char buffer[40] = "";
#else
  wchar_t buffer[40] = L"";
#endif  /* !(__MINGW32__) */
  sem_set_t* ss = NULL;
#endif  /* !(__unix) */

#if (__unix)
  semid = semget(key, num_locks, IPC_CREAT|IPC_EXCL|0666);
  if (semid == -1) {
    printf("create_sem failed errno=%s\n",strerror(errno));
    rc = FALSE;
  } else {
    /* initialize the semaphore */
    sbuf.sem_num = 0;
    sbuf.sem_op = 2;
    sbuf.sem_flg = 0;
    if (semop(semid,&sbuf,1) == -1) {
      printf("create_sem failed during init errno=%s\n",strerror(errno));
      rc = FALSE;
    }
  }
#else
  /* Allocate semaphore set */
  ss = sys_alloc_sem_set(num_locks);
  if(NULL == ss) {
    rc = FALSE;
  }

  for(i = 0; i < num_locks; i++) {
    /* Use ASCII representation of key for object name */
#if (__MINGW32__)
    sprintf(buffer,"%d_%d",key,i);
	/* Semaphore created in "free" state */
    ss->sem[i] = CreateSemaphore(NULL,1,1,buffer);
    if (ss->sem[i] == NULL) {
      printf("create_sem failed errno=%s\n",strerror(EINVAL));
      rc = FALSE;
    }
#else
    swprintf_s(buffer,40,L"%d_%d",key,i);
	/* Semaphore created in "free" state */
    ss->sem[i] = CreateSemaphore(NULL,1,1,buffer);
    if (ss->sem[i] == (int)NULL) {
	  char buf[80];
	  strerror_s(buf,80,EINVAL);
      printf("create_sem failed errno=%s\n",buf);
	  sys_release_sem_set(ss);
      rc = FALSE;
	}
#endif  /* !(__MINGW32__) */
  }
  semid = (int)ss;
#endif  /* !(__unix) */

  return rc;
}

int unlock_sem() {
  int rc = TRUE;
  int member = 0;
#if (__unix)
  struct sembuf sem_unlock={ member, 1, 0};
#else
  sem_set_t* ss = (sem_set_t*)semid;
#endif  /* !(__unix) */
  
  /* Attempt to unlock the semaphore set */
#if (__unix)
  if((semop(semid, &sem_unlock, 1)) == -1) {
    printf("unlock_sem failed errno=%s\n",strerror(errno));
    rc = FALSE;
  }
#else
  if(!ReleaseSemaphore(ss->sem[member],1,NULL)) {
    DWORD dw = GetLastError();
    LPVOID lpMsgBuf;
#if !(__MINGW32__)
    size_t returnValue = 0;
    DWORD lpnSize = 0;
    char buf[80];
#endif  /* !(__MINGW32__) */
	FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
#if (__MINGW32__)
    printf("unlock_sem failed errno=%s\n",(char*)lpMsgBuf);
#else
	lpnSize = wcslen(lpMsgBuf);
    wcstombs_s(&returnValue,buf,sizeof(buf),lpMsgBuf,lpnSize);
    printf("unlock_sem failed errno=%s\n",buf);
#endif  /* (__MINGW32__) */
    LocalFree(lpMsgBuf);
    rc = FALSE;
  }
#endif  /* !(__unix) */
  return rc;
}


int lock_sem() {
  int rc = TRUE;
  int member = 0;
#if (__unix)
  struct sembuf sem_lock={ member, -1, IPC_NOWAIT};
#else
#if !(__MINGW32__)
  char buf[80];
#endif  /* !(__MINGW32__) */
  DWORD dwWaitResult = 0;
  sem_set_t* ss = (sem_set_t*)semid;
#endif  /* !(__unix) */
#if (__unix)
  if((semop(semid, &sem_lock, 1)) == -1) {
    printf("lock_sem failed errno=%s\n",strerror(errno));
    rc = FALSE;
  }
#else
  dwWaitResult = WaitForSingleObject(ss->sem[member],0);
  switch(dwWaitResult)
  {
  case WAIT_TIMEOUT:
    rc = -1;
    errno = EAGAIN;
    break;
  case WAIT_FAILED:
  case WAIT_ABANDONED:
    rc = -1;
    errno = EINVAL;
    break;
  }
#if (__MINGW32__)
  if (rc == -1) {
    printf("lock_sem failed errno=%s\n",strerror(errno)); 
    rc = FALSE;
  }
#else
  strerror_s(buf,80,errno);
  if (rc == -1) {
    printf("lock_sem failed errno=%s\n",buf); 
    rc = FALSE;
  }
#endif  /* !(__MINGW32__) */
#endif  /* !(__unix) */
  return rc;
}

int free_sem() {
  int rc = TRUE;
  int r;
#if (__unix)
  r = semctl( semid, 0, IPC_RMID,0 );
#else
  r = sys_release_sem_set((sem_set_t*)semid);
#endif  /* !(__unix) */
  if (r==-1) {
#if (__unix||__MINGW32__)
    printf("free_sem failed errno=%s\n",strerror(errno));
#else
    char buf[80];
    strerror_s(buf,80,errno);
    printf("free_sem failed errno=%s\n",buf);
#endif  /* !(__unix||__MINGW32__) */
    rc =FALSE;
  }
  return rc;
}

int create_and_attach_shm() {
  int rc = TRUE;
  key_t shmemkey = 0xbabecafe;
  size_t size = 1024;
#if (__unix)
  shmid = shmget(shmemkey, size, 0666 | IPC_CREAT | IPC_EXCL);
  if (shmid == -1) {
    printf("shm create (shget) failed errno=%s\n",strerror(errno));
    rc = FALSE;;
  } else {
    struct shmid_ds dsbuf;
    addr = shmat(shmid, 0, 0);
    if ((long)addr == (-1)) {
      printf("shm create (shmat) failed errno=%s\n",strerror(errno));
      rc = FALSE;
    } else {
      if (shmctl(shmid, IPC_STAT, &dsbuf)) {
        printf("shm create (shctl) failed errno=%s\n",strerror(errno));
        rc = FALSE;
        shmdt(addr);
      } else {
        /* if we are the first to attach, then initialize the segment to 0 */
        if (dsbuf.shm_nattch == 1) {
          memset(addr,0,dsbuf.shm_segsz);
        }        
      }
    }
  }
#else
    /* Use ASCII representation of key for file mapping object name */
#if (__MINGW32__)
	char name[80] = "";
	sprintf(name,"%d",shmemkey);
#else
    char buf[80];
	wchar_t name[80] = L"";
	swprintf(name,80,L"0x%x",shmemkey);
#endif  /* !(__MINGW32__) */
    shmid = (int)CreateFileMapping((HANDLE)INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,size,name);
	if((int)NULL == shmid) {
      shmid = -1;
      errno = EINVAL;
#if (__MINGW32__)
      printf("shm create failed errno=%s\n",strerror(errno));
#else
      strerror_s(buf,80,errno);
      printf("shm create failed errno=%s\n",buf);
#endif  /* !(__MINGW32__) */
    }
    else {
      errno = 0;
      shmid = (int)OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,name);
	  if((int)NULL == shmid) {
        shmid = -1;
        errno = EINVAL;
#if (__MINGW32__)
        printf("shm attach failed errno=%s\n",strerror(errno));
#else
        strerror_s(buf,80,errno);
        printf("shm attach failed errno=%s\n",buf);
#endif  /* !(__MINGW32__) */
      }
      else {
        addr = MapViewOfFile((HANDLE)shmid,FILE_MAP_WRITE,0,0,0);
        if(NULL == addr) {
          CloseHandle((HANDLE)shmid);
          addr = NULL;
        }
      }
    }
#endif  /* !(__unix) */
  return rc;
}

int free_shm() {
  int rc = TRUE;
#if (__unix)
  struct shmid_ds shmid_struct;
#endif  /* (__unix) */
  
  // detach
#if (__unix)
  rc = shmdt(addr); 
  if (rc==-1)  {
    printf("free_shm (shmdt) failed errno=%s\n",strerror(errno));
    rc = FALSE;
  }
  
  /* delete the shared memory id */
  rc = shmctl(shmid, IPC_RMID, &shmid_struct);
  if (rc==-1)  {
    printf("free_shm (shmctl) failed errno=%s\n",strerror(errno));
    rc = FALSE;
  }
#else
  if(!UnmapViewOfFile(addr)) {
#if (__MINGW32__)
    printf("free_shm failed errno=%s\n",strerror(EINVAL));
#else
    char buf[80];
	strerror_s(buf,80,EINVAL);
    printf("free_shm failed errno=%s\n",buf);
#endif  /* (__MINGW32__) */
    rc = FALSE;
  }
  CloseHandle((HANDLE)shmid);
#endif  /* !(__unix) */
  return rc;
}

 
int main() {
  int* a;
  int rc = TRUE;
  printf("creating sem...\n");
  rc |= create_sem();
  printf("locking sem...\n");
  rc |= lock_sem();
  printf("creating and attaching to shmem...\n");
  rc |= create_and_attach_shm();
  a = (int*)addr;
  // make sure we can read/write the shared memory
  *a = 1;
  if (*a != 1) {
    printf("writing shared memory failed\n");
    rc |= 1;
  } else {
    printf("Wrote %i to shared memory\n",*a);
  }
  printf("freeing resources...\n");
  rc |= free_shm();
  rc |= unlock_sem();
  rc |= free_sem();
  return rc;
}

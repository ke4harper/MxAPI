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

#if !(__unix__)
  typedef struct {
    int key;
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
  int rc = -1;
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
    rc = 0;
  }
  return rc;
}

/***************************************************************************
  Function: sys_alloc_sem_set

  Description:

  Parameters:

  Returns: pointer to semaphore set or NULL

 ***************************************************************************/
sem_set_t* sys_alloc_sem_set(int key,int num_locks) {
  sem_set_t* ss = NULL;
  if(0 < num_locks) { /* must have at least one lock,
                       * based on semget behavior */
    ss = (sem_set_t*)malloc(sizeof(sem_set_t));
    if(NULL != ss) {
      memset(ss,0,sizeof(sem_set_t));
      ss->key = key;
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
  }
  return NULL;
}
#endif  /* !(__unix__) */

/***************************************************************************
  Function: sys_sem_create

  Description:

  Parameters:

  Returns: boolean indicating success or failure

 ***************************************************************************/
mrapi_boolean_t sys_sem_create(int key,int num_locks,int* semid) {
#if !(__unix__)
  int i = 0;
#if (__MINGW32__)
  char buffer[40] = "";
#else
  wchar_t buffer[40] = L"";
#endif  /* !(__MINGW32__) */
  sem_set_t* ss = NULL;
#endif  /* !(__unix__) */

  int max_semaphores_per_array = 250;
  /* Wish I could just get this from an include somewhere...*/
  /* To find SEMMSL kernel parameter:
    $ ipcs -ls

    ------ Semaphore Limits --------
    max number of arrays = 128
    max semaphores per array = 250
    max semaphores system wide = 32000
    max ops per semop call = 32
    semaphore max value = 32767
  */
  if (num_locks > max_semaphores_per_array) {
    printf("sys_sem_create failed: num_locks requested is greater then the OS supports (SEMMSL).\n");
    return MRAPI_FALSE;
  }

  mrapi_dprintf(1,"sys_sem_create (create)");
  /* 1. create the semaphore */
#if (__unix__)
  *semid = semget((key_t)key, num_locks, IPC_CREAT|IPC_EXCL|0666);
  if (*semid == -1) {
    mrapi_dprintf(1,"sys_sem_create failed: errno=%s",strerror(errno));
    return MRAPI_FALSE;
  }
#else
  /* Allocate semaphore set */
  ss = sys_alloc_sem_set(key,num_locks);
  if(NULL == ss) {
    return MRAPI_FALSE;
  }

  for(i = 0; i < num_locks; i++) {
    /* Use ASCII representation of key for object name */
#if (__MINGW32__)
    sprintf(buffer,"%u_%u",key,i);
	/* Semaphore created in "free" state */
    ss->sem[i] = CreateSemaphore(NULL,1,1,buffer);
    if (ss->sem[i] == NULL) {
      mrapi_dprintf(1,"sys_sem_create failed: errno=%s",strerror(EINVAL));
#else
    /* Local session prefix allows sharing across processes */
    swprintf_s(buffer,40,L"Local\\mca_%u_%u",key,i);
	/* Semaphore created in "free" state */
    ss->sem[i] = CreateSemaphore(NULL,1,1,buffer);
    if (ss->sem[i] == (int)NULL) {
	  char buf[80];
	  strerror_s(buf,80,EINVAL);
      mrapi_dprintf(1,"sys_sem_create failed: errno=%s",buf);
#endif  /* !(__MINGW32__) */
	  sys_release_sem_set(ss);
	  return MRAPI_FALSE;
	}
  }
  *semid = (int)ss;
#endif  /* !(__unix__) */
  mrapi_dprintf(1,"sys_sem_create (initialize)");

#if (__unix__)
  /* 2. initialize all members (Note: create and initialize are NOT atomic!
     This is why semget must poll to make sure the sem is done with
     initialization */

  struct sembuf sb;
  sb.sem_op = 1;
  sb.sem_flg = 0;

  for(sb.sem_num = 0; sb.sem_num < num_locks; sb.sem_num++) {
    /* do a semop() to "free" the semaphores. */
    /* this sets the sem_otime field, as needed below. */
    if (semop(*semid, &sb, 1) == -1) {
      int e = errno;
      semctl(*semid, 0, IPC_RMID); /* clean up */
      errno = e;
      return MRAPI_FALSE; /* error, check errno */
    }
  }
#endif  /* (__unix__) */

  return MRAPI_TRUE;
}

/***************************************************************************
  Function: sys_sem_get

  Description:

  Parameters:

  Returns: boolean indicating success or failure

***************************************************************************/
mrapi_boolean_t sys_sem_get(int key, int num_locks, int* semid) {

#if (__unix__)
  union semun {
    int val;
    struct semid_ds *buf;
    short *array;
  } arg;
  int ready = 0;
  int MAX_RETRIES = 0xffff;
#else
#if (__MINGW32__)
  char buffer[40];
#else
  wchar_t buffer[40];
#endif  /* (__MINGW32__) */
  sem_set_t* ss = NULL;
#endif  /* !(__unix__) */
  int i;

#if (__unix__)
  struct semid_ds buf;
#endif  /* (__unix__) */

  mrapi_dprintf(1,"sys_sem_get");

#if (__unix__)
  *semid = semget(key, num_locks, 0); /* get the id */
  if (*semid == -1) {
    mrapi_dprintf(1,"sys_sem_get failed: errno=%s",strerror(errno));
    return MRAPI_FALSE;
  }
#else
  ss = sys_alloc_sem_set(key,num_locks);
  if(NULL == ss) {
    return MRAPI_FALSE;
  }

  for(i = 0; i < num_locks; i++) {
    /* Use ASCII representation of key for object name */
    /* TODO: Consider using DuplicateHandle to reduce resource utilization? */
#if (__MINGW32__)
    sprintf(buffer,"Local\\mca_%u_%u",key,i);
    ss->sem[i] = OpenSemaphore(SYNCHRONIZE|SEMAPHORE_MODIFY_STATE,FALSE,buffer);
    if (ss->sem[i] == NULL) {
      mrapi_dprintf(1,"sys_sem_get failed: errno=%s",strerror(EINVAL));
#else
    swprintf_s(buffer,40,L"Local\\mca_%u_%u",key,i);
    ss->sem[i] = OpenSemaphore(SYNCHRONIZE|SEMAPHORE_MODIFY_STATE,FALSE,buffer);
    if (ss->sem[i] == (int)NULL) {
	  char buf[80];
	  strerror_s(buf, 80, EINVAL);
      mrapi_dprintf(1,"sys_sem_get failed: errno=%s",buf);
#endif  /* (__MINGW32__) */
	  sys_release_sem_set(ss);
      return MRAPI_FALSE;
	}
  }
  *semid = (int)ss;
#endif  /* !(__unix__) */
#if (__unix__)
  /*
  At that point, process 2 will have to wait until the semaphore is initialized
  by process 1. How can it tell? Turns out, it can repeatedly call semctl() with
  the IPC_STAT flag, and look at the sem_otime member of the returned struct
  semid_ds structure. If that's non-zero, it means process 1 has performed an
  operation on the semaphore with semop(), presumably to initialize it.
  */

  /* wait for other process to initialize the semaphore: */
  arg.buf = &buf;
  for(i = 0; i < MAX_RETRIES && !ready; i++) {
    semctl(*semid, num_locks-1, IPC_STAT, arg);
    if (arg.buf->sem_otime != 0) {
      ready = 1;
    } else {
      sleep(1);
    }
  }
  if (!ready) {
    errno = ETIME;
    return MRAPI_FALSE;
  }
#endif  /* (__unix__) */

  return MRAPI_TRUE;
}

/***************************************************************************
  Function: sys_sem_duplicate

  Description:

  Parameters:

  Returns: boolean indicating success or failure

***************************************************************************/
mrapi_boolean_t sys_sem_duplicate(int pproc, int psemid, int* semid) {
#if (__unix__)
  /* Global key used across processes, no duplication necessary */
  *semid = psemid;
#else
  int i;
  sem_set_t* pss = NULL;
  sem_set_t* ss = NULL;
  HANDLE proc = GetCurrentProcess();

  mrapi_dprintf(1,"sys_sem_duplicate");

  pss = (sem_set_t*)psemid;
  if(NULL == pss) {
    return MRAPI_FALSE;
  }

  ss = sys_alloc_sem_set(pss->key,pss->num_locks);
  if(NULL == ss) {
    return MRAPI_FALSE;
  }

  for(i = 0; i < ss->num_locks; i++) {
    /* Inheritable handle with same access rights as parent */
    if(0 == DuplicateHandle((HANDLE)pproc,pss->sem[i],proc,&ss->sem[i],0,TRUE,DUPLICATE_SAME_ACCESS)) {
      sys_release_sem_set(ss);
      return MRAPI_FALSE;
	}
  }
  *semid = (int)ss;
#endif  /* !(__unix__) */

  return MRAPI_TRUE;
}

/***************************************************************************
  Function: sys_sem_trylock

  Description: This version of trylock will retry if another thread/process
    has the lock but will not retry if the lock has gone bad (been deleted).

  Parameters:

  Returns: boolean indicating success or failure

 ***************************************************************************/
  mrapi_boolean_t sys_sem_trylock (int semid,int member) {
    int rc = 0;
#if (__unix__)
    struct sembuf sem_lock={ member, -1, IPC_NOWAIT};
#else
#if !(__MINGW32__)
	char buf[80];
#endif  /* !(__MINGW32__) */
    DWORD dwWaitResult = 0;
	sem_set_t* ss = (sem_set_t*)semid;
#endif  /* !(__unix__) */
    mrapi_dprintf(1,"sys_sem_trylock");
    // retry only if we get EINTR
    while (1) {
#if (__unix__)
      rc = semop(semid, &sem_lock, 1);
      if (rc >= 0) {
        return MRAPI_TRUE;
      }
      if ((rc == -1) && (errno != EINTR)) {
        mrapi_dprintf(3,"sys_sem_trylock failed: errno=%s",strerror(errno));
        return MRAPI_FALSE;
      }
      mrapi_dprintf(6,"sys_sem_trylock attempt failed: errno=%s",strerror(errno));
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
      if (rc >= 0) {
        return MRAPI_TRUE;
      }
      if ((rc == -1) && (errno != EINTR)) {
        mrapi_dprintf(3,"sys_sem_trylock failed: errno=%s",strerror(errno));
        return MRAPI_FALSE;
      }
      mrapi_dprintf(6,"sys_sem_trylock attempt failed: errno=%s",strerror(errno));
#else
      if (rc >= 0) {
        return MRAPI_TRUE;
      }
      strerror_s(buf,80,errno);
      if ((rc == -1) && (errno != EINTR)) {
        mrapi_dprintf(3,"sys_sem_trylock failed: errno=%s",buf);
        return MRAPI_FALSE;
      }
      mrapi_dprintf(6,"sys_sem_trylock attempt failed: errno=%s",buf);
#endif  /* !(__MINGW32__) */
#endif  /* !(__unix__) */
	  break;
    }

    return MRAPI_FALSE;
  }

/***************************************************************************
  Function: sys_sem_lock

  Description:

  Parameters:

  Returns: boolean indicating success or failure

 ***************************************************************************/
  mrapi_boolean_t sys_sem_lock (int semid,int member) {

  while(1) {
    // repeatedly call trylock until we get the lock or fail due to an
    // error other than EAGAIN (someone else has the lock).
    if (sys_sem_trylock(semid,member)) {
      return MRAPI_TRUE;
    } else if (errno != EAGAIN) {
#if (__unix__||__MINGW32__)
      mrapi_dprintf(2,"sys_sem_lock attempt failed: errno=%s",strerror(errno));
#else
      char buf[80];
	  strerror_s(buf,80,errno);
      mrapi_dprintf(2,"sys_sem_lock attempt failed: errno=%s",buf);
#endif  /* !(__unix__||__MINGW32__) */
    }
    else {
      sys_os_yield();
    }
  }
  return MRAPI_FALSE;
}

/***************************************************************************
  Function: sys_sem_unlock

  Description:

  Parameters:

  Returns: boolean indicating success or failure

 ***************************************************************************/
  mrapi_boolean_t sys_sem_unlock (int semid,int member) {
#if (__unix__)
  struct sembuf sem_unlock={ member, 1, 0};
#else
  sem_set_t* ss = (sem_set_t*)semid;
#endif  /* !(__unix__) */
  mrapi_dprintf(4,"sys_sem_unlock");
#if (__unix__)
  if((semop(semid, &sem_unlock, 1)) == -1) {
    mrapi_dprintf(1,"sys_sem_unlock failed: errno=%s",strerror(errno));
    return MRAPI_FALSE;
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
    mrapi_dprintf(1,"sys_sem_unlock failed: errno=%s",lpMsgBuf);
#else
	lpnSize = wcslen(lpMsgBuf);
    wcstombs_s(&returnValue,buf,sizeof(buf),lpMsgBuf,lpnSize);
    mrapi_dprintf(1,"sys_sem_unlock failed: errno=%s",buf);
#endif  /* (__MINGW32__) */
    LocalFree(lpMsgBuf);
    return MRAPI_FALSE;
  }
#endif  /* !(__unix__) */
  return MRAPI_TRUE;
}

/***************************************************************************
  Function: sys_sem_release

  Description: releases the semaphore resources for the given id,
               complementing a corresponding get

  Parameters:

  Returns: boolean indicating success or failure

 ***************************************************************************/
mrapi_boolean_t sys_sem_release(int semid){
  mrapi_boolean_t rc = MRAPI_TRUE;
  /* Unix does not have separate resources associated with a get */
#if !(__unix__)
  sem_set_t* ss = (sem_set_t*)semid;
  (void)sys_release_sem_set(ss);
#endif  /* !(__unix__) */
  mrapi_dprintf(1,"sys_sem_release deallocating semaphore:%d",semid);
  return rc;
}

/***************************************************************************
  Function: sys_sem_delete

  Description: detaches and frees the semaphore for the given id.

  Parameters:

  Returns: boolean indicating success or failure

 ***************************************************************************/
mrapi_boolean_t sys_sem_delete(int semid){
  mrapi_boolean_t rc = MRAPI_TRUE;
  int r = -1;
#if (__unix__)
  r = semctl( semid, 0, IPC_RMID,0 );
#else
  sem_set_t* ss = (sem_set_t*)semid;
  r = sys_release_sem_set(ss);
#endif  /* !(__unix__) */
  mrapi_dprintf(1,"sys_sem_delete removing semaphore:%d",semid);
  if (r == -1) {
#if (__unix||__MINGW32__)
    printf("ERROR: unable to remove semaphore: semctl() remove id failed errno: %s\n",strerror(errno));
#else
    char buf[80];
    strerror_s(buf,80,errno);
    printf("ERROR: unable to remove semaphore: semctl() remove id failed errno: %s\n",buf);
#endif  /* !(__unix||__MINGW32__) */
    rc = MRAPI_FALSE;
  }
  return rc;
}

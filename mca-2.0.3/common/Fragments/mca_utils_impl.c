/*

Copyright(c) 2012, ABB, Inc
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :
*Redistributions of source code must retain the above copyright
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
DISCLAIMED.IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Port to Windows: #if !(__unix__||__MINGW32__), etc.
Added timestamp measurement

*/

  /***************************************************************************
  Function: mca_print_tid

  Description:

  Parameters:

  Returns: none
  ***************************************************************************/
const char *mca_print_tid(pthread_t t) {
  static char buffer[100];
  char *p = buffer;

#ifdef __linux
  /* We know that pthread_t is an unsigned long */
  sprintf(p, "%lu", t);
#elif (__unix__||__MINGW32__)
  /* Just print out the contents of the pthread_t */ {
    char *const tend = (char *) ((&t)+1);
    char *tp = (char *) &t;
    while (tp < tend) {
      p += sprintf (p, "%02x", *tp);
      tp++;
      if (tp < tend)
        *p++ = ':';
    }

  }
#else
  sprintf_s(buffer, 100, "%lu", GetCurrentThreadId());
#endif  /* !(__linux||__unix__||__MINGW32__) */
  return buffer;
}

#if !(__unix__||__MINGW32__)
static int cs_init = FALSE;
static CRITICAL_SECTION cs_printf = { 0 };
#endif  /* !(__unix__||__MINGW32__) */

  /***************************************************************************
  Function: mca_dprintf

  Description:

  Parameters:

  Returns: none
  ***************************************************************************/
#if MCA_WITH_DEBUG
inline void mca_dprintf(int level,const char *format, ...) {
  if (level <= debug){
    va_list ap;
#if (__unix__||__MINGW32__)
    pthread_t tid = pthread_self();
#else
    pthread_t tid = GetCurrentThread();
    if(!InterlockedCompareExchange((long*)&cs_init,TRUE,FALSE)) {
      InitializeCriticalSectionAndSpinCount(&cs_printf,0x00000400);
    }
	EnterCriticalSection(&cs_printf);
#endif  /* !(__unix__||__MINGW32__) */
    va_start(ap,format);
#if (__unix__||__MINGW32__)
    printf ("/* MCA PID:%d TID: %s */   //",getpid(),mca_print_tid(tid));
#else
    printf ("/* MCA PID:%d TID: %s */   //",GetCurrentProcessId(),mca_print_tid(tid));
#endif  /* !(__unix__||__MINGW32__) */
    /* call variatic printf */
    vprintf(format,ap);
    printf ("\n");
    va_end(ap);
#if !(__unix__||__MINGW32__)
    LeaveCriticalSection(&cs_printf);
#endif  /* !(__unix__||__MINGW32__) */
  }
}
#else
inline void mca_dprintf(int level,const char *format, ...) {}
#endif


  /***************************************************************************
  Function: mca_set_debug_level

  Description: Sets the debug level which controls verbosity.

  Parameters: d - the desired level

  Returns: none
  ***************************************************************************/
  void mca_set_debug_level (int d)
  {
    if (!MCA_WITH_DEBUG) {
      fprintf(stderr,"ERROR mca_set_debug_level : This library was built without debug support.\n");
      fprintf(stderr,"  If you want to enable debugging, re-build with the --enable-debug option.\n");
    } else {
      debug = d;
    }
  }



  /***************************************************************************
  Function: Crc32_ComputeBuf() - computes the CRC-32 value of a memory buffer

  Description:
      Computes or accumulates the CRC-32 value for a memory buffer.
      The 'inCrc32' gives a previously accumulated CRC-32 value to allow
      a CRC to be generated for multiple sequential buffer-fuls of data.
      The 'inCrc32' for the first buffer must be zero.

  Parameters:
      inCrc32 - accumulated CRC-32 value, must be 0 on first call
      buf     - buffer to compute CRC-32 value for
      bufLen  - number of bytes in buffer

  Returns: crc32 - computed CRC-32 value
  ***************************************************************************/

unsigned long mca_Crc32_ComputeBuf( unsigned long inCrc32, const void *buf,
                                       size_t bufLen )
{
    static const unsigned long crcTable[256] = {
   0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,
   0x9E6495A3,0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,
   0xE7B82D07,0x90BF1D91,0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,
   0x6DDDE4EB,0xF4D4B551,0x83D385C7,0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,
   0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,0x3B6E20C8,0x4C69105E,0xD56041E4,
   0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,0x35B5A8FA,0x42B2986C,
   0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,0x26D930AC,
   0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,
   0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,
   0xB6662D3D,0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,
   0x9FBFE4A5,0xE8B8D433,0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,
   0x086D3D2D,0x91646C97,0xE6635C01,0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,
   0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,0x65B0D9C6,0x12B7E950,0x8BBEB8EA,
   0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,0x4DB26158,0x3AB551CE,
   0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,0x4369E96A,
   0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
   0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,
   0xCE61E49F,0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,
   0xB7BD5C3B,0xC0BA6CAD,0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,
   0x9DD277AF,0x04DB2615,0x73DC1683,0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,
   0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,0xF00F9344,0x8708A3D2,0x1E01F268,
   0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,0xFED41B76,0x89D32BE0,
   0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,0xD6D6A3E8,
   0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,
   0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,
   0x4669BE79,0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,
   0x220216B9,0x5505262F,0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB36A04,0xC2D7FFA7,
   0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,
   0x9C0906A9,0xEB0E363F,0x72076785,0x05005713,0x95BF4A82,0xE2B87A14,0x7BB12BAE,
   0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,0x86D3D2D4,0xF1D4E242,
   0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,0x88085AE6,
   0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45,
   0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,
   0x3E6E77DB,0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,
   0x47B2CF7F,0x30B5FFE9,0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,
   0xCDD70693,0x54DE5729,0x23D967BF,0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,
   0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D };
    unsigned long crc32;
    unsigned char *byteBuf;
    size_t i;

    /** accumulate crc32 for buffer **/
    crc32 = inCrc32 ^ 0xFFFFFFFF;
    byteBuf = (unsigned char*) buf;
    for (i=0; i < bufLen; i++) {
        crc32 = (crc32 >> 8) ^ crcTable[ (crc32 ^ byteBuf[i]) & 0xFF ];
    }
    return( crc32 ^ 0xFFFFFFFF );
}

#if !(__unix__)
int sigprocmask (int how, const sigset_t *set, sigset_t *oset) {
  int rc = 0;
  sigset_t newset = { 0 };
  if(NULL == set) {
    rc = -1;
  }
  else {
    if(NULL != oset) {
#if (__MINGW32__)
      (void)__sync_lock_test_and_set(oset,sigblocked);
#else
      InterlockedExchange(oset,sigblocked);
#endif  /* (__MINGW32__) */
    }
    switch(how) {
    case SIG_SETMASK:
      newset = *set;
      break;
    case SIG_BLOCK:
      newset |= *set;
      break;
    case SIG_UNBLOCK:
      newset &= ~*set;
      break;
    default:
      rc = -1;
    }
    if(!rc) {
#if (__MINGW32__)
      (void)__sync_lock_test_and_set(&sigblocked,newset);
#else
      InterlockedExchange(&sigblocked,newset);
#endif  /* (__MINGW32__) */
    }
  }
  return rc;
}

sigset_t* siggetblocked() {
    return &sigblocked;
}
#endif  /* !(__unix__) */

void mca_block_signals() {
  sigset_t block_alarm;

  /* Initialize the signal mask. */
  sigemptyset (&block_alarm);
#if (__unix__)
  sigaddset (&block_alarm, SIGALRM);
#endif  /* (__unix__) */
  sigaddset (&block_alarm, SIGINT);
#if (__unix__)
  sigaddset (&block_alarm, SIGHUP);
#endif  /* (__unix__) */
  sigaddset (&block_alarm, SIGILL);
  sigaddset (&block_alarm, SIGSEGV);
  sigaddset (&block_alarm, SIGTERM);
  sigaddset (&block_alarm, SIGFPE);
  sigaddset (&block_alarm, SIGABRT);
  sigprocmask (SIG_BLOCK, &block_alarm, NULL);
}

void mca_unblock_signals() {
  sigset_t block_alarm;

  /* Initialize the signal mask. */
  sigemptyset (&block_alarm);
#if (__unix__)
  sigaddset (&block_alarm, SIGALRM);
#endif  /* (__unix__) */
  sigaddset (&block_alarm, SIGINT);
#if (__unix__)
  sigaddset (&block_alarm, SIGHUP);
#endif  /* (__unix__) */
  sigaddset (&block_alarm, SIGILL);
  sigaddset (&block_alarm, SIGSEGV);
  sigaddset (&block_alarm, SIGTERM);
  sigaddset (&block_alarm, SIGFPE);
  sigaddset (&block_alarm, SIGABRT);

  sigprocmask (SIG_UNBLOCK, &block_alarm, NULL);
}

static int MCA_MAGIC = 0x1234;

void mca_begin_ts(mca_timestamp_t* ts) {
  if(NULL != ts) {
#if !(__unix__)
    DWORD_PTR oldMask = 0;
    LARGE_INTEGER qpf = { 0 };
    memset(ts,0,sizeof(mca_timestamp_t));

    /* Always collect the frequency and timestamp from the same CPU */
    oldMask = SetThreadAffinityMask(GetCurrentThread(), 0);
    QueryPerformanceFrequency(&qpf);
    ts->freq = (double)(qpf.QuadPart/1.0E6); /* microseconds */
    QueryPerformanceCounter(&ts->start);
    SetThreadAffinityMask(GetCurrentThread(), oldMask);
#else
    /* Get clock ID and timestamp for this process */
    memset(ts,0,sizeof(mca_timestamp_t));
    (void)clock_getcpuclockid(0,&ts->clock_id);
    (void)clock_gettime(ts->clock_id,&ts->start);
#endif  /* (__unix__) */
    ts->magic = MCA_MAGIC;
    ts->split_samples = 0;
    ts->split_sum = 0.0;
  }
}

void mca_begin_split_ts(mca_timestamp_t* ts) {
  if(NULL != ts) {
    if(MCA_MAGIC != ts->magic) {
      mca_begin_ts(ts);
    }
#if !(__unix__)
    {
      DWORD_PTR oldMask = 0;

      /* Always collect the frequency and timestamp from the same CPU */
      oldMask = SetThreadAffinityMask(GetCurrentThread(), 0);
      QueryPerformanceCounter(&ts->split_start);
      SetThreadAffinityMask(GetCurrentThread(), oldMask);
      }
#else
    (void)clock_gettime(ts->clock_id,&ts->split_start);
#endif  /* (__unix__) */
  }
}

double mca_end_split_ts(mca_timestamp_t* ts) {
  double elapsed = 0.0;
  if(NULL != ts && MCA_MAGIC == ts->magic) {
#if !(__unix__)
    if(0 < ts->split_start.QuadPart) {
      LARGE_INTEGER split_start = { 0 };
      DWORD_PTR oldMask = 0;

      /* Always collect the frequency and timestamp from the same CPU */
      oldMask = SetThreadAffinityMask(GetCurrentThread(), 0);
      QueryPerformanceCounter(&split_start);
      SetThreadAffinityMask(GetCurrentThread(), oldMask);

      elapsed = (double)(split_start.QuadPart - ts->split_start.QuadPart)/ts->freq;
      ts->split_start = split_start;
    }
#else
    if(0 < ts->split_start.tv_nsec) {
      struct timespec split_start = { 0 };
      long int tmp = 0L;

      (void)clock_gettime(ts->clock_id,&split_start);
      tmp = split_start.tv_nsec - ts->split_start.tv_nsec;
      if(0 > tmp) {
          elapsed = 1.0E9*(split_start.tv_sec - ts->split_start.tv_sec-1);
          elapsed += 1.0E9+(double)tmp;
      }
      else {
          elapsed = 1.0E9*(split_start.tv_sec - ts->split_start.tv_sec);
          elapsed += (double)tmp;
      }
      elapsed /= 1.0E3; /* microseconds */
      ts->split_start = split_start;
    }
#endif  /* (__unix__) */
    if(0.0 < elapsed) {
      ts->split_samples++;
      ts->split_sum += elapsed;
    }
  }
  return elapsed;
}

double mca_end_ts(mca_timestamp_t* ts) {
  double elapsed = 0;
  if(NULL != ts && MCA_MAGIC == ts->magic) {
#if !(__unix__)
    LARGE_INTEGER finish = { 0 };
    if(0 < ts->start.QuadPart)
    {
      DWORD_PTR oldMask = 0;

      /* Always collect counters from the same CPU. */
      oldMask = SetThreadAffinityMask(GetCurrentThread(), 0);
      QueryPerformanceCounter(&finish);
      SetThreadAffinityMask(GetCurrentThread(), oldMask);

      elapsed = (double)(finish.QuadPart - ts->start.QuadPart)/ts->freq;
    }
#else
    struct timespec finish = { 0 };
    long int tmp = 0L;
    if(0 != ts->start.tv_nsec) {
      (void)clock_gettime(ts->clock_id,&finish);
      tmp = finish.tv_nsec - ts->start.tv_nsec;
      if(0 > tmp) {
          elapsed = 1.0E9*(finish.tv_sec - ts->start.tv_sec-1);
          elapsed += 1.0E9+(double)tmp;
      }
      else {
          elapsed = 1.0E9*(finish.tv_sec - ts->start.tv_sec);
          elapsed += (double)tmp;
      }
      elapsed /= 1.0E3; /* microseconds */
    }
#endif  /* (__unix__) */
  }
  return elapsed;
}

#if (__unix__)
void mca_read_util(FILE* file,int i,mca_utilization_t* u) {
  char line[128] = "";
  static char fmt[20] = " %Ld %Ld %Ld %Ld";
  char format[128] = "";

  fgets(line,128,file);
  strcpy(format,"cpu");
  if(0 <= i) {
    sprintf(&format[strlen(format)],"%d",i);
  }
  strcat(format,fmt);
  sscanf(line,format,&u->user,&u->userlow,&u->sys,&u->idle);
}
#endif  /* (__unix__) */

double mca_split_util(mca_cpu_t* cpu) {
  double util = 0.0;
  int i = 0;
#if !(__unix__)
  int err = 0;
  PDH_STATUS status = PdhCollectQueryData(cpu->query);
  DWORD ret = 0;
  PDH_FMT_COUNTERVALUE pdhValue = { 0 };
  double value[MCA_MAX_CPUS+1] = { 0.0 };

  for(i = -1; i < (int)cpu->processors; i++) {
    if(ERROR_SUCCESS != PdhGetFormattedCounterValue(cpu->counter[i+1],PDH_FMT_DOUBLE|PDH_FMT_NOCAP100,&ret,&pdhValue)) {
      err = 1;
    }
    else {
      value[i+1] = pdhValue.doubleValue;
    }
  }
  if(!err) {
    for(i = -1; i < (int)cpu->processors; i++) {
      if(0 > i) {
        util = value[i+1];
      }
      cpu->split_sum[i+1] += value[i+1];
    }
    cpu->split_samples++;
  }
#else
  mca_utilization_t* ss = cpu->split_start;
  FILE* file = NULL;

  file = fopen("/proc/stat","r");
  for(i = -1; i < (int)cpu->processors; i++) {
    mca_utilization_t u = { 0 };
    mca_read_util(file,i,&u);
    if(0 < cpu->split_samples[i+1]) {
      double total = 0.0;
      double percent = 0.0;
      /* check for overflow */
      if(u.user < ss[i+1].user ||
         u.userlow < ss[i+1].userlow ||
         u.sys < ss[i+1].sys ||
         u.idle < ss[i+1].idle) {
        continue;
      }

      total = (u.user-ss[i+1].user) + (u.userlow-ss[i+1].userlow) + (u.sys-ss[i+1].sys);
      percent = total;
      total += (u.idle-ss[i+1].idle);
      if(0.0 < total) {
        percent /= total;
        percent *= 100.0;
        if(0 > i) {
          util = percent;
        }
        cpu->split_samples[i+1]++;
        cpu->split_sum[i+1] += percent;
      }

      ss[i+1] = u;
    }
  }
  fclose(file);
#endif  /* (__unix__) */
  return util;
}

void mca_begin_cpu(mca_cpu_t* cpu) {
  if(NULL != cpu) {
    int i = 0;
    memset(cpu,0,sizeof(mca_cpu_t));
    cpu->magic = MCA_MAGIC;
#if !(__unix__)
    {
      SYSTEM_INFO info = { 0 };
      PDH_STATUS status = 0;

      GetSystemInfo(&info);
      cpu->processors = info.dwNumberOfProcessors;
      assert(MCA_MAX_CPUS >= cpu->processors);

      status = PdhOpenQuery(NULL,0,&cpu->query);
    }
    for(i = -1; i < (int)cpu->processors; i++) {
      TCHAR szCounter[80] = TEXT("");
#if (__MINGW32__)
#else
      if(0 > i) {
        _tcscpy_s(szCounter,sizeof(szCounter)/sizeof(TCHAR),TEXT("\\Processor(_Total)\\% Processor Time"));
      }
      else {
        _stprintf_s(szCounter,sizeof(szCounter)/sizeof(TCHAR),TEXT("\\Processor(%d)\\%% Processor Time"),i);
      }
#endif  /* !(__MINGW32__) */
      assert(ERROR_SUCCESS == PdhAddCounter(cpu->query,szCounter,0,&cpu->counter[i+1]));
    }
    SleepEx(10,0); /* give OS a chance to register counters */
    PdhCollectQueryData(cpu->query);
#else
    {
      FILE* file = NULL;
      char line[128] = "";

      file = fopen("/proc/cpuinfo","r");
      while(fgets(line,128,file) != NULL) {
        if(0 == strncmp(line,"processor",9)) {
          cpu->processors++;
        }
      }
      fclose(file);
    }
    assert(MCA_MAX_CPUS >= cpu->processors);

    (void)mca_split_util(cpu);
    for(i = -1; i < (int)cpu->processors; i++) {
      cpu->split_samples[i+1] = 1;
    }
    memcpy(cpu->start,cpu->split_start,(cpu->processors+1)*sizeof(mca_utilization_t));
#endif  /* (__unix__) */
    cpu->split_samples++;
  }
}

double mca_split_cpu(mca_cpu_t* cpu) {
  double util = 0.0;
  if(NULL != cpu) {
    if(MCA_MAGIC != cpu->magic) {
      mca_begin_cpu(cpu);
    }
    util = mca_split_util(cpu);
  }
  return util;
}

double mca_end_cpu(mca_cpu_t* cpu) {
  double util = 0.0;
  if(NULL != cpu && MCA_MAGIC == cpu->magic) {
#if !(__unix__)
    SleepEx(10,0); /* give OS a chance to update counters */
    (void)mca_split_util(cpu);
    PdhCloseQuery(cpu->query);
    util = cpu->split_sum[0]/cpu->split_samples;
#else
    memcpy(cpu->split_start,cpu->start,(cpu->processors+1)*sizeof(mca_utilization_t));
    (void)mca_split_util(cpu);
    util = cpu->split_sum[0]/cpu->split_samples[0];
#endif  /* (__unix__) */
  }
  return util;
}

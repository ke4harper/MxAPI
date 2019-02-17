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

typedef struct
{
    long iterations;
    double run;
    double rundown;
    double util;
} mrapi_elapsed_t;

typedef struct
{
    mrapi_msg_t msg; // must be first member
    int mode;
    int benable;
    int nread;
    int nwrite;
    mca_timestamp_t start;
    mca_timestamp_t start_rundown;
    mrapi_elapsed_t elapsed;
    mca_cpu_t cpu;
    mrapi_msg_t buffer[SYNC_BUFFERS];
} mrapi_sync_t;

typedef struct
{
    pthread_t tid;
    mrapi_sync_t sync[3];
} mrapi_thread_t;

typedef struct
{
    mrapi_msg_t msg; // must be first member
    pid_t pid;
    int nthread;
    int nmode[3];
    mrapi_thread_t thread[SYNC_THREADS];
} mrapi_proc_t;

typedef struct
{
    int nprocess;
    mrapi_proc_t process[SYNC_PROCESSES];
} mrapi_test_db_t;

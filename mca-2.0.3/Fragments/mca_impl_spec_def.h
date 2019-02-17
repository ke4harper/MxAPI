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
* Neither the name of ABB, Inc nor the names of its contributors may be used
to endorse or promote products derived from this software without specific
prior written permission.

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
Added timestamp structure

*/

/*
 * MCA implementation specific type definitions.
 */
#define MCA_MAX_REQUESTS 1024
#define MCA_MAX_CPUS 8
typedef unsigned mca_request_t;

/* real time measurement */
typedef struct {
  int magic;
  uint64_t split_samples;
  double split_sum;
#if !(__unix__)
  double freq;
  LARGE_INTEGER start;
  LARGE_INTEGER split_start;
#else
  clockid_t clock_id;
  struct timespec start;
  struct timespec split_start;
#endif  /* (__unix__) */
} mca_timestamp_t;

#if (__unix__)
typedef struct {
  uint64_t user;
  uint64_t userlow;
  uint64_t sys;
  uint64_t idle;
} mca_utilization_t;
#endif  /* (__unix__) */

typedef struct {
  int magic;
  uint64_t processors;
  double split_sum[MCA_MAX_CPUS+1];
#if !(__unix__)
  HQUERY query;
  HCOUNTER counter[MCA_MAX_CPUS+1];
  uint64_t split_samples;
#else
  mca_utilization_t start[MCA_MAX_CPUS+1];
  mca_utilization_t split_start[MCA_MAX_CPUS+1];
  uint64_t split_samples[MCA_MAX_CPUS+1];
#endif  /* (__unix__) */
} mca_cpu_t;

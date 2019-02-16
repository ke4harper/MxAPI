/*
Copyright (c) 2012, ABB, Inc
All rights reserved.

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

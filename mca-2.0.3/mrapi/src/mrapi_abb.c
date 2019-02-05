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

Copyright (c) 2012, ABB, Inc
All rights reserved.

Port to Windows: #if !(__unix__||__MINGW32__), etc.

*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <mrapi_abb.h>

#include <string.h>
#include <stdio.h>
#include <mca_utils_abb.h> // for mca_set_debug

void mrapi_set_debug_level(int d) { mca_set_debug_level(d); }

char* mrapi_display_status (mrapi_status_t status,char* status_message, size_t size) {
  // Implementation moved to mrapi_impl/mrapi_impl_spec.c
  return mrapi_impl_display_status(status,status_message,size);
}

#include "Fragments/mrapi_initialize.c"
#include "Fragments/mrapi_mutex.c"
#include "Fragments/mrapi_sem.c"
#include "Fragments/mrapi_rwl.c"
#include "Fragments/mrapi_shmem.c"
#include "Fragments/mrapi_rmem.c"
#include "Fragments/mrapi_vtime.c"
#include "Fragments/mrapi_resources.c"
#include "Fragments/mrapi_atomic.c"

#ifdef __cplusplus
extern } 
#endif /* __cplusplus */

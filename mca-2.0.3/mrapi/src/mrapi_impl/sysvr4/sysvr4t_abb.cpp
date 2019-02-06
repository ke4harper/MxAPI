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

// sysvr4t_abb.cpp : Defines the entry point for the console application.
//

#include "Fragments/sysvr4t_inc_abb.cpp"
#include "Fragments/sysvr4t_type_abb.cpp"
#include "Fragments/sysvr4t_threads_abb.cpp"

#if !(__unix__)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif  // (__unix__)
{
    int bproc = (1 < argc);
    int nproc = 1;
    int start = 0;
#if !(__unix__)
    _TCHAR opt = 0;
    _TCHAR* param = NULL;
#else
    char opt = 0;
    char* param = NULL;
#endif  // (__unix__)
    while(argc > start) {
        if(sys_os_getopt(argc, argv, &start, &opt, &param)) {
#if !(__unix__)
            switch(towlower(opt)) {
            case L'p': // -p <proc_num>
                swscanf_s(param,L"%d",&nproc);
                break;
            }
#else
            switch(tolower(opt)) {
            case 'p': // -p <proc_num>
                sscanf(param,"%d",&nproc);
                break;
            }
#endif  // (__unix__)
        }
    }

    mca_set_debug_level(0);

if(!bproc) {
#include "Fragments/sysvr4t_os.cpp"
#include "Fragments/sysvr4t_key.cpp"
#include "Fragments/sysvr4t_sem.cpp"
#include "Fragments/sysvr4t_shmem.cpp"
#include "Fragments/sysvr4t_atomic.cpp"
}
#include "Fragments/sysvr4t_stress.cpp"

	return 0;
}

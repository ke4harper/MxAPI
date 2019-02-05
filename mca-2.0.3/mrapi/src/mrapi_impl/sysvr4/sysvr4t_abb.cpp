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

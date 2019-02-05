/* OS specific */
PUBLIC
#if !(__unix__)
mrapi_boolean_t sys_os_getopt(int argc, _TCHAR* argv[], int* start, _TCHAR* opt, _TCHAR** param);
#else
mrapi_boolean_t sys_os_getopt(int argc, char* argv[], int* start, char* opt, char** param);
#endif  // (__unix__)

PUBLIC
void sys_os_yield(void);
PUBLIC
void sys_os_usleep(int usec);

PUBLIC
void sys_os_srand(unsigned int seed);
PUBLIC
int sys_os_rand(void);

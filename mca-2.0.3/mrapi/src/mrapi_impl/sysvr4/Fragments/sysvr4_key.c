/*
Copyright (c) 2012, ABB, Inc
All rights reserved.
*/

/***************************************************************************
  Function: sys_file_key

  Description:

  Parameters:

  Returns: boolean indicating success or failure

 ***************************************************************************/
mrapi_boolean_t sys_file_key(const char* pathname,int proj_id,int* key) {
  mrapi_boolean_t rc = MRAPI_FALSE;
  int newkey = -1;
  if(NULL != key) {
#if (__unix__)
    static char def[] = "/dev/null";
    if(NULL == pathname || 0 >= strlen(pathname)) {
        pathname = def;
    }
    newkey = ftok(pathname,proj_id);
    if((key_t)-1 != newkey) {
      *key = newkey;
      rc = MRAPI_TRUE;
    }
    mrapi_dprintf(1,"sys_file_key: pathname: %s proj_id: %d key: %d",pathname,proj_id,*key);
#else
    /* Use file ID from specified path directory (embedded environment
       variables allowed), XORed with repeated 8 least significant bits
       of proj_id.
    */
    DWORD nLen = 0;
    static char def[] = "%WINDIR%\\system.ini";
#if (__MINGW32__)
    char szPath[MAX_PATH] = "";
    char szLocal[MAX_PATH] = "";
    if(NULL == pathname || 0 >= strlen(pathname)) {
        pathname = def;
    }
    /* Expand environment variables */
    nLen = ExpandEnvironmentStrings(szPath,szLocal,MAX_PATH);
    if(0 != nLen) {
      int i = 0;
      int code = 0;
      int xor = 0;
      /* Get file handle, must be read accessible */
      HANDLE hFile = CreateFile(szLocal,
                                FILE_READ_ATTRIBUTES,0,NULL,
                                OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
#else
    size_t len = 0;
    wchar_t wszPath[MAX_PATH] = L"";
    wchar_t wszLocal[MAX_PATH] = L"";
    if(NULL == pathname || 0 >= strlen(pathname)) {
        pathname = def;
    }
    mbstowcs_s(&len,wszPath,MAX_PATH,pathname,strlen(pathname)+1);
    /* Expand environment variables */
    nLen = ExpandEnvironmentStrings(wszPath,wszLocal,MAX_PATH);
    if(0 != nLen) {
      int i = 0;
      int code = 0;
      int xor = 0;
      /* Get file handle, must be read accessible */
      HANDLE hFile = CreateFile(wszLocal,
                                FILE_READ_ATTRIBUTES,0,NULL,
                                OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
#endif  /* !(__MINGW32__) */
      if(INVALID_HANDLE_VALUE != hFile) {
        BY_HANDLE_FILE_INFORMATION fileInformation;
        if(GetFileInformationByHandle(hFile,&fileInformation)) {
          /* File is accessible */
          newkey = fileInformation.nFileIndexHigh + fileInformation.nFileIndexLow;
          CloseHandle(hFile);
          /* Mask least significant 8 bits and create XOR value */
          code = proj_id & 0x000000FF;
          for(i = 0; i < 4; i++) {
            xor |= code;
            code <<= 8;
          }
          newkey ^= xor;
        }
      }
    }
    if(-1 != newkey) {
      *key = newkey;
      rc = MRAPI_TRUE;
    }
#endif  /* !(__unix__) */
  }
  return rc;
}

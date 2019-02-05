	// Create file key
	{
        int key1 = 0;
        int key2 = 0;
        // NULL file
        assert(sys_file_key(NULL,'c',&key1));
        assert(0 < key1);
        // Empty file
        assert(sys_file_key("",'c',&key1));
        assert(0 < key1);
        // Inaccessible files
#if !(__unix__)
        assert(!sys_file_key("%SystemDrive%\\pagefile.sys",'c',&key1));
        assert(!sys_file_key("%WINDIR%\\system32",'c',&key1));
        // Negative proj_id
        assert(sys_file_key("%WINDIR%\\system.ini",-1,&key1));
        // Valid file
        assert(sys_file_key("%WINDIR%\\system.ini",'c',&key1));
        assert(0 < key1);
        // Repeatable key
        assert(sys_file_key("%WINDIR%\\system.ini",'c',&key2));
        assert(key2 == key1);
        // File variance
        assert(sys_file_key("%WINDIR%\\win.ini",'d',&key2));
        assert(key2 != key1);
        // proj_id variance
        assert(sys_file_key("%WINDIR%\\system.ini",'d',&key2));
        assert(key2 != key1);
#else
        // Negative proj_id
        assert(sys_file_key("/dev/null",-1,&key1));
        // Valid file
        assert(sys_file_key("/dev/null",'c',&key1));
        assert(0 < key1);
        // Repeatable key
        assert(sys_file_key("/dev/null",'c',&key2));
        assert(key2 == key1);
        // File variance
        assert(sys_file_key("/etc/passwd",'d',&key2));
        assert(key2 != key1);
        // proj_id variance
        assert(sys_file_key("/dev/null",'d',&key2));
        assert(key2 != key1);
#endif  // !(__unix__)
    }

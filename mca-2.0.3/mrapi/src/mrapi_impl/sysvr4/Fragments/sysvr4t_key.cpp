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

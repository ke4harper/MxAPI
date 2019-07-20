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

// Exercise crc generation
	{
		int key;
		char buff[128];
#if (__MINGW32__)
		// Get username and convert to single byte string
		char szName[sizeof(buff)] = "";
		DWORD lpnSize = sizeof(szName);
		GetUserName(szName, &lpnSize);
		strcat(szName, "_mca");
#else
		// Get username and convert to single byte string
		wchar_t szwName[sizeof(buff)] = L"";
		size_t returnValue = 0;
		DWORD lpnSize = sizeof(szwName);
		GetUserName(szwName, &lpnSize);
		wcstombs_s(&returnValue, buff, sizeof(buff), szwName, lpnSize);
		strcat_s(buff, sizeof(buff), "_mca");
#endif  /* !(__unix__||__MINGW32__) */
		/* global key */
		key = 0;
		key = mca_Crc32_ComputeBuf(key, buff, sizeof(buff));
		assert(0 < (unsigned)key);
	}

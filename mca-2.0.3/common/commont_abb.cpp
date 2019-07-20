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

// commont_abb.cpp : Defines the entry point for the console application.
//

#include "Fragments/commont_inc_abb.cpp"

/* https://www.geeksforgeeks.org/sieve-of-eratosthenes/ */

static void SieveOfEratosthenes(int n)
{
	// Create a boolean array "prime[0..n]" and initialize 
	// all entries it as true. A value in prime[i] will 
	// finally be false if i is Not a prime, else true. 
	bool* prime = (bool*)calloc((n + 1), sizeof(bool));
	memset(prime, true, (n + 1) * sizeof(bool));

	for (int p = 2; p*p <= n; p++)
	{
		// If prime[p] is not changed, then it is a prime 
		if (prime[p] == true)
		{
			// Update all multiples of p greater than or  
			// equal to the square of it 
			// numbers which are multiple of p and are 
			// less than p^2 are already been marked.  
			for (int i = p * p; i <= n; i += p)
				prime[i] = false;
		}
	}

	// Print all prime numbers 
	char buffer[20];
	for (int p = 2; p <= n; p++)
		if (prime[p])
#if !(__unix__||VW)
			sprintf_s(buffer, "%i", p);
#else
			sprintf(buffer, "%i", p);
#endif  // (__unix__||VW)

	free(prime);
}

#if !(__unix__)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif  // (__unix__)
{
    mca_set_debug_level(0);

#include "Fragments/commont_debug.cpp"
#include "Fragments/commont_crc.cpp"
#include "Fragments/commont_signal.cpp"
#include "Fragments/commont_timestamp.cpp"
#include "Fragments/commont_cpu.cpp"

	return 0;
}

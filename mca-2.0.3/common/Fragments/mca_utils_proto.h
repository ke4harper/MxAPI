/*

Copyright(c) 2012, ABB, Inc
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :
*Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of ABB, Inc nor the names of its contributors may be used
to endorse or promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Port to Windows: #if !(__unix__||__MINGW32__), etc.
Added timestamp measurement

*/
  const char *mca_print_tid(pthread_t t);
  void mca_dprintf(int level,const char *format, ...);
  unsigned long mca_Crc32_ComputeBuf( unsigned long inCrc32, const void *buf, size_t bufLen );
  void mca_set_debug_level (int d);
  void mca_block_signals();
  void mca_unblock_signals();

  void mca_begin_ts(mca_timestamp_t* ts);
  void mca_begin_split_ts(mca_timestamp_t* ts);
  double mca_end_split_ts(mca_timestamp_t* ts);
  double mca_end_ts(mca_timestamp_t* ts);

  void mca_begin_cpu(mca_cpu_t* cpu);
  double mca_split_cpu(mca_cpu_t* cpu);
  double mca_end_cpu(mca_cpu_t* cpu);

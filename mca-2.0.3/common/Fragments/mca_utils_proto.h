/*
Copyright (c) 2012, ABB, Inc
All rights reserved.

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

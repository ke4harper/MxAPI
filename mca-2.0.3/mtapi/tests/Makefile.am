## Makefile.am -- Process this file with automake to produce Makefile.in

# after all programs are built, run this test script
TESTS = regress

EXTRA_DIST = regress 

check_PROGRAMS = mthread_test1 mtapi_test1 mtapi_test2 mtapi_test3

MRAPI_CFLAGS = -I$(top_srcdir)/mrapi/include 
MCAPI_CFLAGS = -I$(top_srcdir)/mcapi/include -I$(top_srcdir)/mcapi/src/mcapi_trans/mcapi_trans_sm
MCAPI_LTLIBS = ../../mcapi/src/mcapi_trans/mcapi_trans_sm/mrapi/libmcapi.la

if BAREMETAL
MRAPI_LTLIBS = ../../mrapi/src/mrapi_impl/baremetal/powerpc/libmrapi.la
else
MRAPI_LTLIBS = ../../mrapi/src/mrapi_impl/sysvr4/libmrapi.la
endif

AM_CFLAGS =  $(MRAPI_CFLAGS) $(MCAPI_CFLAGS)

#
# the mtapi library
MTAPI_LTLIBS = ../src/libmtapi.la
  
include_HEADERS = $(top_srcdir)/mtapi/include/mtapi.h

# include files & paths
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/common -I$(top_srcdir)/mtapi/include

# TODO -- if we are bare metal then don't want -lpthread
# programs (sources & libraries)
mthread_test1_SOURCES = mthread_test1.c
mthread_test1_LDADD = $(MTAPI_LTLIBS) $(MCAPI_LTLIBS) $(MRAPI_LTLIBS) -lpthread

mtapi_test1_SOURCES = mtapi_test1.c
mtapi_test1_LDADD = $(MTAPI_LTLIBS) $(MCAPI_LTLIBS) $(MRAPI_LTLIBS) -lpthread

mtapi_test2_SOURCES = mtapi_test2.c
mtapi_test2_LDADD = $(MTAPI_LTLIBS) $(MCAPI_LTLIBS) $(MRAPI_LTLIBS) -lpthread


mtapi_test3_SOURCES = mtapi_test3.c
mtapi_test3_LDADD = $(MTAPI_LTLIBS) $(MCAPI_LTLIBS) $(MRAPI_LTLIBS) -lpthread



#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([mca], [2.0.3])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_CONFIG_HEADER([mca_config.h])
# process conker_config through ax_prefix to add prefixs to the defines
# otherwise we get naming conflicts between the different projects (mrapi,mcapi,and conker) 
#AX_PREFIX_CONFIG_H([mca_config.h])

# additional cross-comilation flags  (--build=i686-pc-linux-gnu --host=powerpc-eabispe)
if [[ x$host_alias = xpowerpc-eabispe ]]; then
	SRCDIR=`cd ${srcdir} && pwd`
	CCASFLAGS="-mcpu=603 -DNO_SUPPORT_FOR_SC  -w -Wa,-I./ -Wa,-I${SRCDIR}/tests/cross_compilation_support/ -Wa,-I${SRCDIR}/tests/cross_compilation_support/ind_includes"
	LDFLAGS="-msim -lm -Wl,-T,${SRCDIR}/tests/cross_compilation_support/generic_booke-gcc.lcf"
fi	

CFLAGS=" -ggdb -Wall -Werror ${CSETUP} ${CCASFLAGS}"

# Checks for programs.
AC_PROG_CC
AM_PROG_AS
AC_PROG_INSTALL
AC_PROG_LIBTOOL
  
# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
#AC_TYPE_UINT8_T
#AC_TYPE_UINT16_T
#AC_TYPE_UINT32_T
#AC_TYPE_UINT64_T


# check for CFLAGS (profiling, optimized, efence, etc.)
AM_C_SETUP(2,9)

CFLAGS=" -g -Wall ${CSETUP} ${CCASFLAGS}"
LDFLAGS="${CSETUP}"



# Check if the user wants to build with debug print support
AC_MSG_CHECKING([if the user wants to build with debug print support])
AC_ARG_ENABLE(debug,AC_HELP_STRING([--enable-debug],[Compile in the debug print statements.]),
[case "${enableval}" in
  yes)
        with_debug=true
        ;;
  no)
        with_debug=false
        ;;
  *)
        AC_MSG_ERROR(bad value ${enableval} for --enable-debug)
        ;;
esac],[with_debug=false])
AH_TEMPLATE([MCA_WITH_DEBUG],[Defined and set to 1 if we're including debug print support.])
if [[ $with_debug = true ]] ; then
        AC_MSG_RESULT([yes])
        AC_DEFINE([MCA_WITH_DEBUG],1)
else
        AC_MSG_RESULT([no])
        AC_DEFINE([MCA_WITH_DEBUG],0)
fi

# Check if the user wants to build with baremetal support
# (The AC_DEFINE will add a #define to mrapi_config.h, the AM_CONDITIONAl
#  will make it visible to tests/Makefile.am)
bmval=0
AC_MSG_CHECKING([if the user wants to build for baremetal])
AC_ARG_ENABLE(baremetal,AC_HELP_STRING([--enable-baremetal],[Compile got baremetal.]),
[case "${enableval}" in
  yes)
        mrapi_with_baremetal=true
        bmval=1
        ;;
  no)
        mrapi_with_baremetal=false
        ;;
  *)
        AC_MSG_ERROR(bad value ${enableval} for --enable-baremetal)
        ;;
esac],[mrapi_with_baremetal=false])
AH_TEMPLATE([MRAPI_WITH_BAREMETAL],[Defined and set to 1 if we're compiling for baremetal.])
if [[ $mrapi_with_baremetal = true ]] ; then
        AC_MSG_RESULT([yes])
        AC_DEFINE([MRAPI_WITH_BAREMETAL],1)
else
        AC_MSG_RESULT([no])
        AC_DEFINE([MRAPI_WITH_BAREMETAL],0)
fi
AM_CONDITIONAL([BAREMETAL],[test x${mrapi_with_baremetal} = xtrue])
AC_SUBST(BAREMETAL,${bmval})

# Check if a simulator has been specified (for now we just have adl, add simics)
# (This will fill in the adlsim path in regress_configure.pm)
AdlSim=
AdlExists=yes
# If the user has specified a path to the adl simulator, then remember it
AC_ARG_WITH([adl],AC_HELP_STRING([--with-adl],[specify the path containing the adl simulator.]),ac_adl="${withval}")
# Now try to find the program using the path
AC_PATH_PROG(AdlSim, [elf_v5], no, $ac_adl)
if [[ "$AdlSim" = no ]] ; then
  AC_MSG_WARN([Could not find adl elf_v5 simulator in the path provided in the --with-adl option.])
  AdlExists=no
fi
# If found, then try and run it as a sanity check.
if [[ $AdlExists != no ]] ; then
  AC_MSG_CHECKING([that we can run the adl simulator])
  AdlTest="${AdlSim} -v"
  ${AdlTest} > /dev/null
  if [[ $? = 0 ]]; then
    AC_MSG_RESULT([yes])
  else
    AC_MSG_WARN([no.  Please modify your path or use the --with-adl option.])
    AdlExists=no
  fi
fi
ADL_SIM=${AdlSim}
AC_SUBST(ADL_SIM)

#conker:
# Check if the user wants to build with affinity support
AC_MSG_CHECKING([if the user wants to build with processor affinity support])
AC_ARG_ENABLE(affinity,AC_HELP_STRING([--enable-affinity],[Compile in the sched_set/get_affinity function calls.]),
[case "${enableval}" in
  yes)
        conker_with_affinity=true
        ;;
  no)
        conker_with_affinity=false
        ;;
  *)
        AC_MSG_ERROR(bad value ${enableval} for --enable-affinity)
        ;;
esac],[conker_with_affinity=false])

AH_TEMPLATE([CONKER_WITH_AFFINITY],[Defined and set to 1 if we're including affinity support.])
if [[ $conker_with_affinity = true ]] ; then
        AC_MSG_RESULT([yes])
        AC_DEFINE([CONKER_WITH_AFFINITY],1)
else
        AC_MSG_RESULT([no])
        AC_DEFINE([CONKER_WITH_AFFINITY],0)
fi

      
# conker:
# Check if the user wants to build with gettimeofday
AC_MSG_CHECKING([if the user wants to use gettimeofday])
AC_ARG_ENABLE(gettimeofday,AC_HELP_STRING([--enable-gettimeofday],[Compile to use gettimeofday.]),
[case "${enableval}" in
  yes)
        with_gtod=true
        ;;
  no)
Z        with_gtod=false
        ;;
  *)
        AC_MSG_ERROR(bad value ${enableval} for --enable-gettimeofday)
        ;;
esac],[with_gtod=false])

AH_TEMPLATE([CONKER_WITH_GETTIMEOFDAY],[Defined and set to 1 if we're compiling to use gettimeofday.])
if [[ $with_gtod = true ]] ; then
        AC_MSG_RESULT([yes])
        AC_DEFINE([CONKER_WITH_GETTIMEOFDAY],1)
else
        AC_MSG_RESULT([no])
        AC_DEFINE([CONKER_WITH_GETTIMEOFDAY],0)
fi

## Check for MAX_* defines


## Check for MAX_* defines
##define MTAPI_MAX_TASKS 64 
AC_MSG_CHECKING([checking for mtapi_max_tasks])
ac_mtapi_max_tasks_default=64
mtapi_max_tasks=
AC_ARG_WITH([mtapi_max_tasks],AC_HELP_STRING([--with-mtapi_max_tasks],[specify a mtapi_max_tasks.  The default is $ac_mtapi_max_tasks_default.]),mtapi_max_tasks=$withval,mtapi_max_tasks=$ac_mtapi_max_tasks_default)
AH_TEMPLATE([MTAPI_MAX_TASKS],[Defined and set to $mtapi_max_tasks.])
AC_MSG_RESULT([$mtapi_max_tasks])
AC_DEFINE_UNQUOTED([MTAPI_MAX_TASKS],[$mtapi_max_tasks])

## Check for MAX_* defines
##define MAX_MSG_SIZE 1024
AC_MSG_CHECKING([checking for max_msg_size])
ac_max_msg_size_default=1024
max_msg_size=
AC_ARG_WITH([max_msg_size],AC_HELP_STRING([--with-max_msg_size],[specify a max_msg_size.  The default is $ac_max_msg_size_default.]),max_msg_size=$withval,max_msg_size=$ac_max_msg_size_default)
AH_TEMPLATE([MCAPI_MAX_MSG_SIZE],[Defined and set to $max_msg_size.])
AC_MSG_RESULT([$max_msg_size])
AC_DEFINE_UNQUOTED([MCAPI_MAX_MSG_SIZE],[$max_msg_size])

##define MAX_PKT_SIZE 1024 
AC_MSG_CHECKING([checking for max_pkt_size])
ac_max_pkt_size_default=1024
max_pkt_size=
AC_ARG_WITH([max_pkt_size],AC_HELP_STRING([--with-max_pkt_size],[specify a max_pkt_size.  The default is $ac_max_pkt_size_default.]),max_pkt_size=$withval,max_pkt_size=$ac_max_pkt_size_default)
AH_TEMPLATE([MCAPI_MAX_PKT_SIZE],[Defined and set to $max_pkt_size.])
AC_MSG_RESULT([$max_pkt_size])
AC_DEFINE_UNQUOTED([MCAPI_MAX_PKT_SIZE],[$max_pkt_size])

#define MAX_ENDPOINTS 32
AC_MSG_CHECKING([checking for max_endpoints])
ac_max_endpoints_default=32
max_endpoints=
AC_ARG_WITH([max_endpoints],AC_HELP_STRING([--with-max_endpoints],[specify a max_endpoints.  The default is $ac_max_endpoints_default.]),max_endpoints=$withval,max_endpoints=$ac_max_endpoints_default)
AH_TEMPLATE([MCAPI_MAX_ENDPOINTS],[Defined and set to $max_endpoints.])
AC_MSG_RESULT([$max_endpoints])
AC_DEFINE_UNQUOTED([MCAPI_MAX_ENDPOINTS],[$max_endpoints])

#define MAX_ATTRIBUTES 2
AC_MSG_CHECKING([checking for max_attributes])
ac_max_attributes_default=2
max_attributes=
AC_ARG_WITH([max_attributes],AC_HELP_STRING([--with-max_attributes],[specify a max_attributes.  The default is $ac_max_attributes_default.]),max_attributes=$withval,max_attributes=$ac_max_attributes_default)
AH_TEMPLATE([MCAPI_MAX_ATTRIBUTES],[Defined and set to $max_attributes.])
AC_MSG_RESULT([$max_attributes])
AC_DEFINE_UNQUOTED([MCAPI_MAX_ATTRIBUTES],[$max_attributes])

#define MAX_CHANNELS 32 
AC_MSG_CHECKING([checking for max_channels])
ac_max_channels_default=32
max_channels=
AC_ARG_WITH([max_channels],AC_HELP_STRING([--with-max_channels],[specify a max_channels.  The default is $ac_max_channels_default.]),max_channels=$withval,max_channels=$ac_max_channels_default)
AH_TEMPLATE([MCAPI_MAX_CHANNELS],[Defined and set to $max_channels.])
AC_MSG_RESULT([$max_channels])
AC_DEFINE_UNQUOTED([MCAPI_MAX_CHANNELS],[$max_channels])

#define MAX_NODES 16
AC_MSG_CHECKING([checking for max_nodes])
ac_max_nodes_default=16
max_nodes=
AC_ARG_WITH([max_nodes],AC_HELP_STRING([--with-max_nodes],[specify a max_nodes.  The default is $ac_max_nodes_default.]),max_nodes=$withval,max_nodes=$ac_max_nodes_default)
AH_TEMPLATE([MCA_MAX_NODES],[Defined and set to $max_nodes.])
AC_MSG_RESULT([$max_nodes])
AC_DEFINE_UNQUOTED([MCA_MAX_NODES],[$max_nodes])

#define MAX_DOMAINS (2)
AC_MSG_CHECKING([checking for max_domains])
ac_max_domains_default=2
max_buffers=
AC_ARG_WITH([max_domains],AC_HELP_STRING([--with-max_domains],[specify a max_domains.  The default is $ac_max_domains_default.]),max_domains=$withval,max_domains=$ac_max_domains_default)
AH_TEMPLATE([MCA_MAX_DOMAINS],[Defined and set to $max_domains.])
AC_MSG_RESULT([$max_domains])
AC_DEFINE_UNQUOTED([MCA_MAX_DOMAINS],[$max_domains])

#define MAX_BUFFERS 128 
AC_MSG_CHECKING([checking for max_buffers])
ac_max_buffers_default=1284
max_buffers=
AC_ARG_WITH([max_buffers],AC_HELP_STRING([--with-max_buffers],[specify a max_buffers.  The default is $ac_max_buffers_default.]),max_buffers=$withval,max_buffers=$ac_max_buffers_default)
AH_TEMPLATE([MCAPI_MAX_BUFFERS],[Defined and set to $max_buffers.])
AC_MSG_RESULT([$max_buffers])
AC_DEFINE_UNQUOTED([MCAPI_MAX_BUFFERS],[$max_buffers])

#define MAX_QUEUE_ELEMENTS 64 
AC_MSG_CHECKING([checking for max_queue_elements])
ac_max_queue_elements_default=64
max_queue_elements=
AC_ARG_WITH([max_queue_elements],AC_HELP_STRING([--with-max_queue_elements],[specify a max_queue_elements.  The default is $ac_max_queue_elements_default.]),max_queue_elements=$withval,max_queue_elements=$ac_max_queue_elements_default)
AH_TEMPLATE([MCAPI_MAX_QUEUE_ELEMENTS],[Defined and set to $max_queue_elements.])
AC_MSG_RESULT([$max_queue_elements])
AC_DEFINE_UNQUOTED([MCAPI_MAX_QUEUE_ELEMENTS],[$max_queue_elements])



AC_CONFIG_FILES([ \
  		Makefile \
                common/Makefile \
		mrapi/Makefile \
		mrapi/scripts/Makefile \
                mrapi/include/Makefile \
                mrapi/doc/Makefile \
                mrapi/src/Makefile \
		mrapi/src/mrapi_impl/Makefile \
                mrapi/src/mrapi_impl/sysvr4/Makefile \
                mrapi/src/mrapi_impl/baremetal/Makefile \
                mrapi/src/mrapi_impl/baremetal/powerpc/Makefile \
                mrapi/tests/Makefile \
                mrapi/tests/regress_configure.pm \
                mcapi/Makefile \
                mcapi/doc/Makefile \
                mcapi/scripts/Makefile \
                mcapi/include/Makefile \
		mcapi/src/Makefile \
		mcapi/src/mcapi_trans/Makefile \
                mcapi/src/mcapi_trans/mcapi_trans_stub/Makefile \
                mcapi/src/mcapi_trans/mcapi_trans_sm/Makefile \
                mcapi/src/mcapi_trans/mcapi_trans_sm/mrapi/Makefile \
                mcapi/tests/Makefile \
                mtapi/Makefile \
                mtapi/scripts/Makefile \
                mtapi/src/Makefile \
                mtapi/include/Makefile \
                mtapi/tests/Makefile \
                conker/Makefile \
                conker/include/Makefile \
                conker/src/Makefile \
                conker/tests/Makefile \
                conker/scripts/Makefile \
		])

AC_CONFIG_FILES([\
	mrapi/scripts/mrapi-config \
],[chmod +x,-w mrapi/scripts/mrapi-config])

AC_CONFIG_FILES([\
        mcapi/scripts/mcapi-config \
],[chmod +x,-w mcapi/scripts/mcapi-config])

AC_CONFIG_FILES([\
        mtapi/scripts/mtapi-config \
],[chmod +x,-w mtapi/scripts/mtapi-config])

AC_OUTPUT

AUTOMAKE_OPTIONS = foreign

include_HEADERS = mca_config.h  mca.h  mca_impl_spec.h

SUBDIRS = common \
          mrapi \
	  mcapi \
          mtapi \
          conker 

#if BUILD_DOCS
#SUBDIRS += doc
#endif

# add path to m4 macros
ACLOCAL_AMFLAGS = -I ./macros

BASEDIR=$(shell cd $(top_builddir) && pwd)

# check if the user wants debug print support
#if [ x$(WithDebug) != x ] ; then \
#     WITH_DEBUG=1 ; \
     WITH_DEBUG=1
#fi

basic:
	$(MAKE) all-local


clean-local:
	rm -rf $(top_builddir)/lib $(top_builddir)/bin 


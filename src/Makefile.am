INCLUDES=-I$(top_srcdir)/include
lib_LTLIBRARIES=libcmalloc-@LIBCMALLOC_API_VERSION@.la
###

libcmalloc_@LIBCMALLOC_API_VERSION@_la_SOURCES=cmalloc.c
libcmalloc_@LIBCMALLOC_API_VERSION@_la_CFLAGS=-Wall #-Werror
libcmalloc_@LIBCMALLOC_API_VERSION@_la_LDFLAGS=-no-undefined -version-info $(LIBCMALLOC_SO_VERSION)
libcmalloc_@LIBCMALLOC_API_VERSION@_la_LIBADD=-lm
libcmalloc_@LIBCMALLOC_API_VERSION@_la_includedir=$(includedir)/libcmalloc-@LIBCMALLOC_API_VERSION@/
libcmalloc_@LIBCMALLOC_API_VERSION@_la_include_HEADERS=$(top_srcdir)/include/libcmalloc-@LIBCMALLOC_API_VERSION@/cmalloc.h

###
###


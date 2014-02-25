INCLUDES=-I$(top_srcdir)/include
lib_LTLIBRARIES=libcp_utils-@LIIBCP_UTILS_API_VERSION@.la
###

libcp_utils_@LIIBCP_UTILS_API_VERSION@_la_SOURCES=cmalloc.c
libcp_utils_@LIIBCP_UTILS_API_VERSION@_la_CFLAGS=-Wall -Wextra -Wformat=2 -Wswitch-default -Wcast-align -Wpointer-arith -Wbad-function-cast -Wstrict-prototypes -Winline -Wundef -Wnested-externs -Wcast-qual -Wshadow -Wwrite-strings  -Wunreachable-code -Wstrict-aliasing=2 -Wno-uninitialized
libcp_utils_@LIIBCP_UTILS_API_VERSION@_la_LDFLAGS=-no-undefined -version-info $(LIIBCP_UTILS_SO_VERSION)
libcp_utils_@LIIBCP_UTILS_API_VERSION@_la_LIBADD=-lm
libcp_utils_@LIIBCP_UTILS_API_VERSION@_la_includedir=$(includedir)/libcp_utils-@LIIBCP_UTILS_API_VERSION@/
libcp_utils_@LIIBCP_UTILS_API_VERSION@_la_include_HEADERS=$(top_srcdir)/include/libcp_utils-@LIIBCP_UTILS_API_VERSION@/cmalloc.h $(top_srcdir)/include/libcp_utils-@LIIBCP_UTILS_API_VERSION@/queue.h $(top_srcdir)/include/libcp_utils-@LIIBCP_UTILS_API_VERSION@/stack.h

###
###


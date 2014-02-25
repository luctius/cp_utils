INCLUDES=-I$(top_srcdir)/include
noinst_PROGRAMS=cmalloc_test queue_test stack_test

cmalloc_test_SOURCES=cmalloc_test.c $(top_srcdir)/src/cmalloc.c
cmalloc_test_CFLAGS=-Wall -Wextra -Wformat=2 -Wswitch-default -Wcast-align -Wpointer-arith -Wbad-function-cast -Wstrict-prototypes -Winline -Wundef -Wnested-externs -Wcast-qual -Wshadow -Wwrite-strings  -Wunreachable-code -Wstrict-aliasing=2 -Wno-uninitialized
cmalloc_test_CPPFLAGS =
cmalloc_test_LDADD = -lm

stack_test_SOURCES=stack_test.c $(top_srcdir)/src/stack.c
stack_test_CFLAGS=-Wall -Wextra -Wformat=2 -Wswitch-default -Wcast-align -Wpointer-arith -Wbad-function-cast -Wstrict-prototypes -Winline -Wundef -Wnested-externs -Wcast-qual -Wshadow -Wwrite-strings  -Wunreachable-code -Wstrict-aliasing=2 -Wno-uninitialized
stack_test_CPPFLAGS =
stack_test_LDADD = -lm

queue_test_SOURCES=queue_test.c $(top_srcdir)/src/queue.c
queue_test_CFLAGS=-Wall -Wextra -Wformat=2 -Wswitch-default -Wcast-align -Wpointer-arith -Wbad-function-cast -Wstrict-prototypes -Winline -Wundef -Wnested-externs -Wcast-qual -Wshadow -Wwrite-strings  -Wunreachable-code -Wstrict-aliasing=2 -Wno-uninitialized
queue_test_CPPFLAGS =
queue_test_LDADD = -lm

TESTS=cmalloc_test stack_test queue_test

INCLUDES=-I$(top_srcdir)/include
noinst_PROGRAMS=cmalloc_test stack_test queue_test dummy_test usb_test example_open_close

cmalloc_test_SOURCES=cmalloc_test.c $(top_srcdir)/utils/cmalloc.c
cmalloc_test_CFLAGS=
cmalloc_test_CPPFLAGS = -I$(top_srcdir)/utils
cmalloc_test_LDADD = $(top_srcdir)/utils/libutils.la -lm

stack_test_SOURCES=stack_test.c $(top_srcdir)/utils/stack.c
stack_test_CFLAGS=
stack_test_CPPFLAGS = -I$(top_srcdir)/utils
stack_test_LDADD = $(top_srcdir)/utils/libutils.la -lm

queue_test_SOURCES=queue_test.c $(top_srcdir)/utils/queue.c
queue_test_CFLAGS=
queue_test_CPPFLAGS = -I$(top_srcdir)/utils
queue_test_LDADD = $(top_srcdir)/utils/libutils.la -lm

dummy_test_SOURCES=dummy_test.c
dummy_test_CFLAGS=
dummy_test_CPPFLAGS =
dummy_test_LDADD = $(top_srcdir)/src/libreadout-2.0.la $(top_srcdir)/drivers/libreadout_dummy_driver-2.0.la -lm

usb_test_SOURCES=usb_test.c
usb_test_CFLAGS=
usb_test_CPPFLAGS =
usb_test_LDADD = $(top_srcdir)/src/libreadout-2.0.la $(top_srcdir)/drivers/libreadout_usb_driver-2.0.la -lm

example_open_close_SOURCES=example_open_close.c
example_open_close_CFLAGS=
example_open_close_CPPFLAGS =
example_open_close_LDADD = $(top_srcdir)/src/libreadout-2.0.la $(top_srcdir)/drivers/libreadout_usb_driver-2.0.la -lm

TESTS=cmalloc_test stack_test queue_test

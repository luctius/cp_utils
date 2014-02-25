#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.60])
AC_INIT(libcp_utils, 1.0.0, corpeters@incas3.eu)
AC_CONFIG_MACRO_DIR([m4])

AC_SUBST([LIBCP_UTILS_SO_VERSION], [1:0:0])
AC_SUBST([LIBCP_UTILS_API_VERSION], [1.0])

AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE([disable])

##########################################################################
# debug compilation support
##########################################################################

AC_MSG_CHECKING([whether to build with debug information])
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug],
                              [enable debug data generation (def=no)])],
                              [debugit="$enableval"],
                              [debugit=no])
AC_MSG_RESULT([$debugit])

if test x"$debugit" = x"yes"; then
    AC_DEFINE([DEBUG],[],[Debug Mode])
    AM_CFLAGS="-g -Wall -Werror -Wno-uninitialized -O0"
    AM_CXXFLAGS="-g -Wall -Werror -Wno-uninitialized -O0"
    AC_MSG_NOTICE([Debug Mode on.])
else
    AC_DEFINE([NDEBUG],[],[No-debug Mode])
    AM_CFLAGS="$AM_CFLAGS -O3"
    AM_CXXFLAGS="$AM_CXXFLAGS -O3"
    AC_MSG_NOTICE([Debug Mode off.])
fi

##########################################################################
# Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_LANG_C
AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_DEFINE([_GNU_SOURCE], 1, [Use GNU extensions])

# Checks for libraries.
# check if math lib exists and contains sine function
AC_CHECK_LIB([math], [pow])
AC_CHECK_LIB([rt], [clock_gettime])
AC_CHECK_LIB([pthread], [sem_init])

#AC_CHECK_PROGS([DOXYGEN], [doxygen])
#if test -z "$DOXYGEN";
#    then AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
#fi
#AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([docs/Doxyfile])])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stddef.h stdint.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_FUNC_MALLOC
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memmove memset])

AC_CONFIG_SRCDIR(src/cmalloc.c)
AC_CONFIG_FILES([
    Makefile 
    src/Makefile 
    test/Makefile 
    cp_utils-${LIBCP_UTILS_API_VERSION}.pc 
    ])

AC_CONFIG_HEADERS([src/def.h])

AC_OUTPUT


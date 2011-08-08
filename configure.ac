#						-*- Autoconf -*-
# Process this file with autotools to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libplist, 0.1)
AM_INIT_AUTOMAKE(libplist, 0.1)
LT_INIT
AC_CONFIG_SRCDIR([src/plist.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h:config.h.in])

# Check for the compilation tools
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

# Check for headers and types
AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_STDBOOL

AC_TYPE_SIZE_T

AC_CONFIG_FILES([Makefile
		 src/Makefile])
AC_OUTPUT

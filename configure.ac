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
AC_LANG(C)
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_PATH_PROG([ATFRUN], [atf-run])
AC_PATH_PROG([ATFREPORT], [atf-report])

# Check for headers and types
AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_STDBOOL

AC_TYPE_SIZE_T

# Check for the Automated Test Framework (atf)
AC_MSG_CHECKING([whether to build atf tests])
AC_ARG_WITH([atf],
	    [AS_HELP_STRING([--with-atf=PATH],
			    [enable support for atf tests])
	     AS_HELP_STRING([--without-atf],
			    [disable support for atf tests])],
	    [with_atf="$withval"])

case "$with_atf" in
  no)
	AC_MSG_RESULT(no)
	;;
  *)
	AC_MSG_RESULT(yes)

	atf=""
	dnl if there is a path, check it
	if test -n "$with_atf"; then
		if test "x$with_atf" != "xyes"; then
			oldLDFLAGS=$LDFLAGS
			oldCPPFLAGS=$CPPFLAGS
			LDFLAGS="$LDFLAGS -L$with_atf/.libs"
			CPPFLAGS="$CPPFLAGS -I$with_atf"
			atf="yes"
			AC_CHECK_LIB(atf-c, atf_no_error, ,
				     atf=""
				     LDFLAGS=$oldLDFLAGS
				     CPPFLAGS=$oldCPPFLAGS)
		fi
	fi

	if test "x$atf" != "xyes"; then
		atf="yes"
		AC_CHECK_LIB(atf-c, atf_no_error, ,
			     atf="")
	fi

	if test "x$atf" = "xyes"; then
		AC_CHECK_HEADERS([atf-c.h])
	fi

	;;
esac
AM_CONDITIONAL([BUILD_TESTS], [test "x$atf" = xyes])

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 tests/Makefile])
AC_OUTPUT

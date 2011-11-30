ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(top_srcdir)/src
LDADD = ../src/libplist.la -latf-c
AM_CFLAGS = -static -g
CLEANFILES =
EXTRA_DIST =

tests_PROGRAMS = t_plist
tests_DATA = Atffile

t_plist_SOURCES = t_plist.c

#
# Derived from ATF autoconf
#

TEST_ENVIRONMENT = PATH=$(prefix)/bin:$${PATH} \
		   PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig

testsdir = $(exec_prefix)/tests
pkgtestsdir = $(testsdir)/$(PACKAGE)

installcheck-local: installcheck-atf
.PHONY: installcheck-atf
installcheck-atf:
	logfile=$$(pwd)/installcheck.log; \
	fifofile=$$(pwd)/installcheck.fifo; \
	cd $(pkgtestsdir); \
	rm -f $${fifofile}; \
	mkfifo $${fifofile}; \
	cat $${fifofile} | tee $${logfile} | \
		$(TESTS_ENVIRONMENT) $(ATFREPORT) & \
		$(TESTS_ENVIRONMENT) $(ATFRUN) >>$${fifofile}; \
	res=$${?}; \
	wait; \
	rm $${fifofile}; \
	echo; \
	echo "The verbatim output of atf-run has been saved to" \
	     "installcheck.log; exit was $${res}"; \
	test $${res} -eq 0
CLEANFILES += installcheck.fifo installcheck.log

pkgtests_DATA = Atffile
EXTRA_DIST += $(pkgtests_DATA)


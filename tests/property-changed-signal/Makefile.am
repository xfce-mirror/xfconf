test_scripts = \
	t-string-changed-signal.sh

check_PROGRAMS = \
	t-string-changed-signal

check_SCRIPTS = $(test_scripts)

TESTS = $(test_scripts)

AM_CFLAGS = \
	-DXFCONFD=\"$(top_builddir)/xfconfd/xfconfd\" \
	-I$(top_srcdir) \
	-I$(top_srcdir)/tests \
	$(GLIB_CFLAGS)

LIBS = \
	$(top_builddir)/xfconf/libxfconf-$(LIBXFCONF_VERSION_API).la

t_string_changed_signal_SOURCES = t-string-changed-signal.c

%.sh: $(top_srcdir)/tests/test-template.sh.in Makefile
	sed -e 's/@TEST_NAME@/$@/; s/\.sh//;' <$(top_srcdir)/tests/test-template.sh.in >$@
	chmod 755 $@

clean-local:
	rm -f t-*-changed-signal.sh

EXTRA_DIST = \
	$(test_scripts)

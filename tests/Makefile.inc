check_SCRIPTS = $(addsuffix .sh,$(check_PROGRAMS))

TESTS = $(check_SCRIPTS)
TESTS_ENVIRONMENT = XDG_CONFIG_HOME="$(top_builddir)/tests/test-xdg_config_home" XFCONFD="$(top_builddir)/xfconfd/xfconfd"

AM_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/tests \
	$(GLIB_CFLAGS) \
	$(DBUS_CFLAGS)

LIBS = \
	$(top_builddir)/xfconf/libxfconf-$(LIBXFCONF_VERSION_API).la

%.sh: $(top_srcdir)/tests/test-template.sh.in Makefile
	sed -e 's/@TEST_NAME@/$@/; s/\.sh//;' <$(top_srcdir)/tests/test-template.sh.in >$@
	chmod 755 $@

CLEANFILES = $(check_SCRIPTS)

test_scripts = \
	t-set-properties.sh \
	t-has-properties.sh \
	t-get-properties.sh \
	t-remove-properties.sh

check_PROGRAMS = \
	t-set-properties \
	t-has-properties \
	t-get-properties \
	t-remove-properties

check_SCRIPTS = $(test_scripts)

TESTS = $(test_scripts)

AM_CFLAGS = \
	-DXFCONFD=\"$(top_builddir)/xfconfd/xfconfd\" \
	-I$(top_srcdir) \
	$(GLIB_CFLAGS)

LIBS = \
	$(top_builddir)/xfconf/libxfconf-$(LIBXFCONF_VERSION_API).la

t_set_properties_SOURCES = t-set-properties.c tests-common.h
t_has_properties_SOURCES = t-has-properties.c tests-common.h
t_get_properties_SOURCES = t-get-properties.c tests-common.h
t_remove_properties_SOURCES = t-remove-properties.c tests-common.h

%.sh: test-template.sh.in Makefile
	sed -e 's/@TEST_NAME@/$@/; s/\.sh//;' <$(srcdir)/test-template.sh.in >$@
	chmod 755 $@

clean-local:
	-rm -rf test-xdg_config_home

EXTRA_DIST = \
	$(test_scripts) \
	test-template.sh.in

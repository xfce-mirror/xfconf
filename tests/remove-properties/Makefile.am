test_scripts = \
	t-remove-string.sh \
	t-remove-int.sh \
	t-remove-uint64.sh \
	t-remove-double.sh \
	t-remove-arrayv.sh \
	t-remove-boolean.sh \
	t-remove-stringlist.sh

check_PROGRAMS = \
	t-remove-string \
	t-remove-int \
	t-remove-uint64 \
	t-remove-double \
	t-remove-arrayv \
	t-remove-boolean \
	t-remove-stringlist

check_SCRIPTS = $(test_scripts)

TESTS = $(test_scripts)

AM_CFLAGS = \
	-DXFCONFD=\"$(top_builddir)/xfconfd/xfconfd\" \
	-I$(top_srcdir) \
	-I$(top_builddir)/tests \
	$(GLIB_CFLAGS)

LIBS = \
	$(top_builddir)/xfconf/libxfconf-$(LIBXFCONF_VERSION_API).la

t_remove_string_SOURCES = t-remove-string.c
t_remove_int_SOURCES = t-remove-int.c
t_remove_uint64_SOURCES = t-remove-uint64.c
t_remove_double_SOURCES = t-remove-double.c
t_remove_arrayv_SOURCES = t-remove-arrayv.c
t_remove_boolean_SOURCES = t-remove-boolean.c
t_remove_stringlist_SOURCES = t-remove-stringlist.c

%.sh: test-template.sh.in Makefile
	sed -e 's/@TEST_NAME@/$@/; s/\.sh//;' <$(srcdir)/test-template.sh.in >$@
	chmod 755 $@

clean-local:
	rm -f t-remove-*.sh

EXTRA_DIST = \
	$(test_scripts) \
	test-template.sh.in

test_scripts = \
	t-set-string.sh \
	t-set-int.sh \
	t-set-uint64.sh \
	t-set-double.sh \
	t-set-arrayv.sh \
	t-set-boolean.sh \
	t-set-stringlist.sh

check_PROGRAMS = \
	t-set-string \
	t-set-int \
	t-set-uint64 \
	t-set-double \
	t-set-arrayv \
	t-set-boolean \
	t-set-stringlist

check_SCRIPTS = $(test_scripts)

TESTS = $(test_scripts)

AM_CFLAGS = \
	-DXFCONFD=\"$(top_builddir)/xfconfd/xfconfd\" \
	-I$(top_srcdir) \
	-I$(top_builddir)/tests \
	$(GLIB_CFLAGS)

LIBS = \
	$(top_builddir)/xfconf/libxfconf-$(LIBXFCONF_VERSION_API).la

t_set_string_SOURCES = t-set-string.c
t_set_int_SOURCES = t-set-int.c
t_set_uint64_SOURCES = t-set-uint64.c
t_set_double_SOURCES = t-set-double.c
t_set_arrayv_SOURCES = t-set-arrayv.c
t_set_boolean_SOURCES = t-set-boolean.c
t_set_stringlist_SOURCES = t-set-stringlist.c

%.sh: test-template.sh.in Makefile
	sed -e 's/@TEST_NAME@/$@/; s/\.sh//;' <$(srcdir)/test-template.sh.in >$@
	chmod 755 $@

clean-local:
	rm -f t-set-*.sh

EXTRA_DIST = \
	$(test_scripts) \
	test-template.sh.in

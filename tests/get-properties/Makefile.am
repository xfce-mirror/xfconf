test_scripts = \
	t-get-string.sh \
	t-get-int.sh \
	t-get-uint64.sh \
	t-get-double.sh \
	t-get-arrayv.sh \
	t-get-boolean.sh \
	t-get-stringlist.sh

check_PROGRAMS = \
	t-get-string \
	t-get-int \
	t-get-uint64 \
	t-get-double \
	t-get-arrayv \
	t-get-boolean \
	t-get-stringlist

check_SCRIPTS = $(test_scripts)

TESTS = $(test_scripts)

AM_CFLAGS = \
	-DXFCONFD=\"$(top_builddir)/xfconfd/xfconfd\" \
	-I$(top_srcdir) \
	-I$(top_srcdir)/tests \
	$(GLIB_CFLAGS)

LIBS = \
	$(top_builddir)/xfconf/libxfconf-$(LIBXFCONF_VERSION_API).la

t_get_string_SOURCES = t-get-string.c
t_get_int_SOURCES = t-get-int.c
t_get_uint64_SOURCES = t-get-uint64.c
t_get_double_SOURCES = t-get-double.c
t_get_arrayv_SOURCES = t-get-arrayv.c
t_get_boolean_SOURCES = t-get-boolean.c
t_get_stringlist_SOURCES = t-get-stringlist.c

%.sh: $(top_srcdir)/tests/test-template.sh.in Makefile
	sed -e 's/@TEST_NAME@/$@/; s/\.sh//;' <$(top_srcdir)/tests/test-template.sh.in >$@
	chmod 755 $@

clean-local:
	rm -f t-get-*.sh

EXTRA_DIST = \
	$(test_scripts)

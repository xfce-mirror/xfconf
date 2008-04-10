test_scripts = \
	t-has-string.sh \
	t-has-int.sh \
	t-has-uint64.sh \
	t-has-double.sh \
	t-has-arrayv.sh \
	t-has-boolean.sh \
	t-has-stringlist.sh

check_PROGRAMS = \
	t-has-string \
	t-has-int \
	t-has-uint64 \
	t-has-double \
	t-has-arrayv \
	t-has-boolean \
	t-has-stringlist

check_SCRIPTS = $(test_scripts)

TESTS = $(test_scripts)

AM_CFLAGS = \
	-DXFCONFD=\"$(top_builddir)/xfconfd/xfconfd\" \
	-I$(top_srcdir) \
	-I$(top_srcdir)/tests \
	$(GLIB_CFLAGS)

LIBS = \
	$(top_builddir)/xfconf/libxfconf-$(LIBXFCONF_VERSION_API).la

t_has_string_SOURCES = t-has-string.c
t_has_int_SOURCES = t-has-string.c
t_has_uint64_SOURCES = t-has-uint64.c
t_has_double_SOURCES = t-has-double.c
t_has_arrayv_SOURCES = t-has-arrayv.c
t_has_boolean_SOURCES = t-has-boolean.c
t_has_stringlist_SOURCES = t-has-stringlist.c

%.sh: $(top_srcdir)/tests/test-template.sh.in Makefile
	sed -e 's/@TEST_NAME@/$@/; s/\.sh//;' <$(top_srcdir)/tests/test-template.sh.in >$@
	chmod 755 $@

clean-local:
	rm -f t-has-*.sh

EXTRA_DIST = \
	$(test_scripts)


noinst_LTLIBRARIES = \
	libxfconf-common.la \
	libxfconf-gvaluefuncs.la

libxfconf_common_la_SOURCES = \
	xfconf-errors.c \
	xfconf-marshal.c \
	xfconf-marshal.h \
	xfconf-types.c \
	xfconf-util.c \
	xfconf-util.h

libxfconf_common_la_CFLAGS = \
	-DLIBXFCONF_COMPILATION \
	-I$(top_srcdir) \
	$(GLIB_CFLAGS)

libxfconf_built_sources = \
	xfconf-alias.h \
	xfconf-aliasdef.c


libxfconf_gvaluefuncs_la_SOURCES = \
	xfconf-gvaluefuncs.c

libxfconf_gvaluefuncs_la_CFLAGS = \
	-DLIBXFCONF_COMPILATION \
	-I$(top_srcdir) \
	$(GLIB_CFLAGS) \
	$(DBUS_GLIB_CFLAGS)


if MAINTAINER_MODE

BUILT_SOURCES = \
	xfconf-marshal.c \
	xfconf-marshal.h

xfconf-marshal.h: stamp-xfconf-marshal.h
	@true
stamp-xfconf-marshal.h: xfconf-marshal.list Makefile
	glib-genmarshal --prefix=xfconf_marshal --internal xfconf-marshal.list --header > xfconf-marshal.h
	echo timestamp > $(@F)
xfconf-marshal.c: xfconf-marshal.list Makefile
	echo '#include "xfconf-marshal.h"' > xfconf-marshal.c
	glib-genmarshal --prefix=xfconf_marshal --internal xfconf-marshal.list --body >> xfconf-marshal.c



CLEANFILES = \
	actual-abi \
	expected-abi

DISTCLEANFILES = \
	$(libxfconf_built_sources)

BUILT_SOURCES += \
	$(libxfconf_built_sources)

xfconf-alias.h: make-exo-alias.pl $(top_srcdir)/xfconf/xfconf.symbols
	$(PERL) $(srcdir)/make-exo-alias.pl <$(top_srcdir)/xfconf/xfconf.symbols >xfconf-alias.h

xfconf-aliasdef.c: make-exo-alias.pl $(top_srcdir)/xfconf/xfconf.symbols
	$(PERL) $(srcdir)/make-exo-alias.pl -def <$(top_srcdir)/xfconf/xfconf.symbols >xfconf-aliasdef.c

endif


EXTRA_DIST = \
	make-exo-alias.pl \
	xfconf-alias.h \
	xfconf-aliasdef.c \
	xfconf-common-private.h \
	xfconf-dbus.xml \
	xfconf-marshal.list

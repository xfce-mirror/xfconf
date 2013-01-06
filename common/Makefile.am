
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DLIBXFCONF_COMPILATION \
	-DG_LOG_DOMAIN=\"xfconf-common\" \
	$(PLATFORM_CPPFLAGS)

noinst_LTLIBRARIES = \
	libxfconf-common.la \
	libxfconf-gvaluefuncs.la

libxfconf_common_la_SOURCES = \
	xfconf-errors.c \
	xfconf-marshal.c \
	xfconf-marshal.h

libxfconf_common_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(PLATFORM_CFLAGS)

libxfconf_common_la_LDFLAGS = \
	$(PLATFORM_LDFLAGS)

libxfconf_common_la_LIBADD = \
	$(GLIB_LIBS)

libxfconf_built_sources = \
	xfconf-alias.h \
	xfconf-aliasdef.c

libxfconf_gvaluefuncs_la_SOURCES = \
	xfconf-gvaluefuncs.c \
	xfconf-gvaluefuncs.h

libxfconf_gvaluefuncs_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(PLATFORM_CFLAGS)

libxfconf_gvaluefuncs_la_LDFLAGS = \
	$(PLATFORM_LDFLAGS)

libxfconf_gvaluefuncs_la_LIBADD = \
	$(GLIB_LIBS) \
	$(DBUS_GLIB_LIBS)


if MAINTAINER_MODE

BUILT_SOURCES = \
	xfconf-marshal.c \
	xfconf-marshal.h

xfconf-marshal.h: stamp-xfconf-marshal.h
	@true
stamp-xfconf-marshal.h: $(srcdir)/xfconf-marshal.list Makefile
	$(AM_V_GEN) glib-genmarshal --prefix=_xfconf_marshal $(srcdir)/xfconf-marshal.list --header > xfconf-marshal.h \
	&& echo timestamp > $(@F)
xfconf-marshal.c: $(srcdir)/xfconf-marshal.list Makefile
	$(AM_V_GEN) echo '#include "xfconf-marshal.h"' > xfconf-marshal.c \
	&& glib-genmarshal --prefix=_xfconf_marshal $(srcdir)/xfconf-marshal.list --body >> xfconf-marshal.c



CLEANFILES = \
	actual-abi \
	expected-abi

DISTCLEANFILES = \
	$(libxfconf_built_sources)

BUILT_SOURCES += \
	$(libxfconf_built_sources)

xfconf-alias.h: make-exo-alias.pl $(top_srcdir)/xfconf/xfconf.symbols
	$(AM_V_GEN) $(PERL) $(srcdir)/make-exo-alias.pl <$(top_srcdir)/xfconf/xfconf.symbols >xfconf-alias.h

xfconf-aliasdef.c: make-exo-alias.pl $(top_srcdir)/xfconf/xfconf.symbols
	$(AM_V_GEN) $(PERL) $(srcdir)/make-exo-alias.pl -def <$(top_srcdir)/xfconf/xfconf.symbols >xfconf-aliasdef.c

endif


EXTRA_DIST = \
	make-exo-alias.pl \
	xfconf-alias.h \
	xfconf-aliasdef.c \
	xfconf-common-private.h \
	xfconf-dbus.xml \
	xfconf-marshal.list \
	xfconf-types.c

# required for make distcheck
dist-hook: all

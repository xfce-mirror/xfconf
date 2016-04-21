
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DLIBXFCONF_COMPILATION \
	-DG_LOG_DOMAIN=\"xfconf\" \
	$(PLATFORM_CPPFLAGS)

lib_LTLIBRARIES = libxfconf-0.la

libxfconfincludedir = $(includedir)/xfce4/xfconf-$(LIBXFCONF_VERSION_API)/xfconf
libxfconfinclude_HEADERS = \
	xfconf-binding.h \
	xfconf-channel.h \
	xfconf-errors.h \
	xfconf-types.h \
	xfconf.h

libxfconf_0_la_SOURCES = \
	$(libxfconfinclude_HEADERS) \
	xfconf-binding.c \
	xfconf-cache.c \
	xfconf-cache.h \
	xfconf-channel.c \
	xfconf-dbus-bindings.h \
	xfconf-gdbus-bindings.c \
	xfconf-private.h \
	xfconf.c \
	$(top_srcdir)/common/xfconf-types.c

libxfconf_0_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
        $(GIO_UNIX_CFLAGS) \
	$(GTHREAD_CFLAGS) \
	$(PLATFORM_CFLAGS)

libxfconf_0_la_LDFLAGS = \
	-export-dynamic \
	-version-info $(LIBXFCONF_VERINFO) \
	-export-symbols-regex "^[^_].*" \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

libxfconf_0_la_LIBADD = \
	$(top_builddir)/common/libxfconf-common.la \
	$(top_builddir)/common/libxfconf-gvaluefuncs.la \
	$(GIO_LIBS) \
        $(GIO_UNIX_LIBS) \
	$(GLIB_LIBS) \
	$(GTHREAD_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxfconf-0.pc


if MAINTAINER_MODE

BUILT_SOURCES = \
	xfconf-gdbus-bindings.c \
	xfconf-gdbus-bindings.h

xfconf-gdbus-bindings.h:
xfconf-gdbus-bindings.c: $(top_srcdir)/common/xfconf-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN) \
	--interface-prefix org.xfce.Xfconf \
	--c-namespace Xfconf   \
	--generate-c-code xfconf-gdbus-bindings \
	$<

CLEANFILES = \
	$(BUILT_SOURCES)

endif

EXTRA_DIST = \
	abicheck.sh \
	xfconf.symbols

# required for gtk-doc
dist-hook: all

if HAVE_GNUC_VISIBILITY
TESTS = abicheck.sh
endif


AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DLIBXFCONF_COMPILATION \
	-DG_LOG_DOMAIN=\"xfconf\" \
	$(PLATFORM_CPPFLAGS)

lib_LTLIBRARIES = libxfconf-1.la

libxfconfincludedir = $(includedir)/xfce4/xfconf-$(LIBXFCONF_VERSION_API)/xfconf
libxfconfinclude_HEADERS = \
	xfconf-binding.h \
	xfconf-channel.h \
	xfconf-errors.h \
	xfconf-types.h \
	xfconf.h

libxfconf_1_la_SOURCES = \
	$(libxfconfinclude_HEADERS) \
	xfconf-binding.c \
	xfconf-cache.c \
	xfconf-cache.h \
	xfconf-channel.c \
	xfconf-private.h \
	xfconf.c \
	$(top_srcdir)/common/xfconf-types.c

libxfconf_1_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
        $(GIO_UNIX_CFLAGS) \
	$(GTHREAD_CFLAGS) \
	$(PLATFORM_CFLAGS)

libxfconf_1_la_LDFLAGS = \
	-export-dynamic \
	-version-info $(LIBXFCONF_VERINFO) \
	-export-symbols-regex "^[^_].*" \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

libxfconf_1_la_LIBADD = \
	$(top_builddir)/common/libxfconf-common.la \
	$(top_builddir)/common/libxfconf-gvaluefuncs.la \
	$(top_builddir)/common/libxfconf-dbus.la \
	$(GIO_LIBS) \
        $(GIO_UNIX_LIBS) \
	$(GLIB_LIBS) \
	$(GTHREAD_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxfconf-1.pc


EXTRA_DIST = \
	abicheck.sh \
	xfconf.symbols

# required for gtk-doc
dist-hook: all

if HAVE_GNUC_VISIBILITY
TESTS = abicheck.sh
endif

if BUILD_XFCONF_GTK

lib_LTLIBRARIES = libxfconf-gtk-0.la

libxfconfgtkincludedir = $(includedir)/xfce4/xfconf-gtk-$(LIBXFCONF_GTK_VERSION_API)/xfconf-gtk
libxfconfgtkinclude_HEADERS = \
	xfconf-gtk.h 

libxfconf_gtk_0_la_SOURCES = \
	$(libxfconfgtkinclude_HEADERS) \
	xfconf-gtk.c

libxfconf_gtk_0_la_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/common \
	-DLIBXFCONF_COMPILATION \
	-DLIBXFCONF_GTK_COMPILATION \
	-DG_LOG_DOMAIN=\"xfconf-gtk\" \
	$(GTK_CFLAGS)

libxfconf_gtk_0_la_LDFLAGS = \
	-export-dynamic \
	-version-info $(LIBXFCONF_GTK_VERINFO) \
	-export-symbols-regex "^[^_].*" \
	-no-undefined

libxfconf_gtk_0_la_LIBADD = \
	$(top_builddir)/common/libxfconf-gvaluefuncs.la \
	$(top_builddir)/xfconf/libxfconf-$(LIBXFCONF_VERSION_API).la \
	$(GTK_LIBS)

libxfconf_gtk_0_la_DEPENDENCIES = \
	$(top_builddir)/xfconf/libxfconf-$(LIBXFCONF_VERSION_API).la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxfconf-gtk-$(LIBXFCONF_GTK_VERSION_API).pc

endif

EXTRA_DIST = \
	libxfconf-gtk-$(LIBXFCONF_GTK_VERSION_API).pc.in

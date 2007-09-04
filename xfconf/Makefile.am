
lib_LTLIBRARIES = libxfconf-0.la

libxfconfincludedir = $(includedir)/xfce4/xfconf-$(LIBXFCONF_VERSION_API)/xfconf
libxfconfinclude_HEADERS = \
	xfconf.h

libxfconf_0_la_SOURCES = \
	$(libxfconfinclude_HEADERS) \
	xfconf.c \
	xfconf-dbus-bindings.h

libxfconf_0_la_CFLAGS = \
	-I$(top_srcdir)/common \
	-DLIBXFCONF_COMPILATION \
	-DG_LOG_DOMAIN=\"xfconf\" \
	$(GLIB_CFLAGS) \
	$(DBUS_CFLAGS)

libxfconf_0_la_LDFLAGS = \
	-export-dynamic \
	-version-info $(LIBXFCONF_VERINFO) \
	-export-symbols-regex "^[^_].*" \
	-no-undefined

libxfconf_0_la_LIBADD = \
	$(GLIB_LIBS) \
	$(DBUS_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxfconf-0.pc

if MAINTAINER_MODE

xfconf-dbus-bindings.h: $(top_srcdir)/common/xfconf-dbus.xml
	dbus-binding-tool --mode=glib-client --prefix=xfconf_client $< \
	    | sed -e 's/org_xfce_Xfconf_/xfconf_client_/g;' \
	          -e 's/_GUI_/_gui_/g;' > $@

endif


lib_LTLIBRARIES = libxfconf-0.la

libxfconfincludedir = $(includedir)/xfce4/xfconf-$(LIBXFCONF_VERSION_API)/xfconf
libxfconfinclude_HEADERS = \
	xfconf-channel.h \
	xfconf.h

libxfconf_0_la_SOURCES = \
	$(libxfconfinclude_HEADERS) \
	xfconf-channel.c \
	xfconf-marshal.c \
	xfconf-marshal.h \
	xfconf-private.h \
	xfconf.c \
	xfconf-dbus-bindings.h

libxfconf_0_la_CFLAGS = \
	-I$(top_srcdir)/common \
	-DLIBXFCONF_COMPILATION \
	-DG_LOG_DOMAIN=\"xfconf\" \
	$(GLIB_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(DBUS_GLIB_CFLAGS)

libxfconf_0_la_LDFLAGS = \
	-export-dynamic \
	-version-info $(LIBXFCONF_VERINFO) \
	-export-symbols-regex "^[^_].*" \
	-no-undefined

libxfconf_0_la_LIBADD = \
	$(GLIB_LIBS) \
	$(DBUS_LIBS) \
	$(DBUS_GLIB_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxfconf-0.pc

if MAINTAINER_MODE

xfconf-marshal.h: stamp-xfconf-marshal.h
	@true
stamp-xfconf-marshal.h: xfconf-marshal.list Makefile
	glib-genmarshal --prefix=xfconf_marshal xfconf-marshal.list --header > xfconf-marshal.h
	echo timestamp > $(@F)
xfconf-marshal.c: xfconf-marshal.list Makefile
	echo '#include "xfconf-marshal.h"' > xfconf-marshal.c
	glib-genmarshal --prefix=xfconf_marshal xfconf-marshal.list --body >> xfconf-marshal.c

xfconf-dbus-bindings.h: $(top_srcdir)/common/xfconf-dbus.xml
	dbus-binding-tool --mode=glib-client --prefix=xfconf_client $< \
	    | sed -e 's/org_xfce_Xfconf_/xfconf_client_/g;' \
	          -e 's/_GUI_/_gui_/g;' > $@

endif

EXTRA_DIST = xfconf-marshal.list

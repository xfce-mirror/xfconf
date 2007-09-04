
bin_PROGRAMS = xfconfd

xfconf_backend_sources =

if BUILD_XFCONF_BACKEND_PERCHANNEL_XML
xfconf_backend_sources += \
	xfconf-backend-perchannel-xml.c \
	xfconf-backend-perchannel-xml.h
endif

xfconfd_SOURCES = \
	main.c \
	xfconf-backend-factory.c \
	xfconf-backend-factory.h \
	xfconf-backend.c \
	xfconf-backend.h \
	xfconf-daemon.c \
	xfconf-daemon.h \
	xfconfd-dbus-server.h \
	$(xfconf_backend_sources)

xfconfd_CFLAGS = \
	-DLOCALEDIR=\"$(localedir)\" \
	-I$(top_srcdir)/common \
	$(GLIB_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS)

xfconfd_LDADD = \
	$(GLIB_LIBS) \
	$(DBUS_LIBS) \
	$(DBUS_GLIB_LIBS) \
	$(LIBXFCE4UTIL_LIBS)


if MAINTAINER_MODE

xfconfd-dbus-server.h: $(top_srcdir)/common/xfconf-dbus.xml
	dbus-binding-tool --mode=glib-server --prefix=xfconfd $< > $@

endif


EXTRA_DIST = \
	xfconf-backend-perchannel-xml.c \
	xfconf-backend-perchannel-xml.h

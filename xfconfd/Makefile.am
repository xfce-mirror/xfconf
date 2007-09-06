
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
	xfconf-dbus-server.h \
	$(xfconf_backend_sources)

xfconfd_CFLAGS = \
	-DBINDIR=\"$(bindir)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-I$(top_srcdir)/common \
	$(GLIB_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS)

xfconfd_LDADD = \
	$(top_builddir)/common/libxfconf-common.la \
	$(GLIB_LIBS) \
	$(DBUS_LIBS) \
	$(DBUS_GLIB_LIBS) \
	$(LIBXFCE4UTIL_LIBS)


servicedir = $(datadir)/dbus-1/services
service_in_files = org.xfce.Xfconf.service.in
service_DATA = $(service_in_files:.service.in=.service)

%.service: %.service.in
	sed -e "s,\@bindir\@,$(bindir),g" < $< > $@

if MAINTAINER_MODE

xfconf-dbus-server.h: $(top_srcdir)/common/xfconf-dbus.xml
	dbus-binding-tool --mode=glib-server --prefix=xfconf $< > $@

endif


EXTRA_DIST = \
	xfconf-backend-perchannel-xml.c \
	xfconf-backend-perchannel-xml.h \
	$(service_in_files)

DISTCLEANFILES = \
	$(service_DATA)

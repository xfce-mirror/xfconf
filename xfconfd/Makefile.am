
libexec_PROGRAMS = xfconfd

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
	xfconf-locking-utils.c \
	xfconf-locking-utils.h \
	xfconf-proptree.c \
	xfconf-proptree.h \
	$(xfconf_backend_sources) \
	$(top_srcdir)/common/xfconf-types.c

xfconfd_CFLAGS = \
	-DG_LOG_DOMAIN=\"xfconfd\" \
	-DLIBXFCONF_COMPILATION \
	-DBINDIR=\"$(bindir)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-I$(top_srcdir) \
	-I$(top_srcdir)/common \
	-I$(top_builddir) \
	-I$(top_builddir)/common \
	$(GLIB_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS)

if ENABLE_PROFILING
xfconfd_CFLAGS += -pg
endif

xfconfd_LDADD = \
	$(top_builddir)/common/libxfconf-common.la \
	$(top_builddir)/common/libxfconf-gvaluefuncs.la \
	$(GLIB_LIBS) \
	$(DBUS_LIBS) \
	$(DBUS_GLIB_LIBS) \
	$(LIBXFCE4UTIL_LIBS)


servicedir = $(datadir)/dbus-1/services
service_in_files = org.xfce.Xfconf.service.in
service_DATA = $(service_in_files:.service.in=.service)

%.service: %.service.in
	sed -e "s,\@libexecdir\@,$(libexecdir),g" < $< > $@

CLEANFILES = \
	$(service_DATA)

EXTRA_DIST = \
	xfconf-backend-perchannel-xml.c \
	xfconf-backend-perchannel-xml.h \
	$(service_in_files)


if MAINTAINER_MODE

xfconf_built_sources = \
	xfconf-dbus-server.h

DISTCLEANFILES = \
	$(xfconf_built_sources)

BUILT_SOURCES = \
	$(xfconf_built_sources)

xfconf-dbus-server.h: $(top_srcdir)/common/xfconf-dbus.xml
	dbus-binding-tool --mode=glib-server --prefix=xfconf $< > $@

endif

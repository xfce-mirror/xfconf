
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfconf-query\" \
	-DDATADIR=\"$(datadir)\" \
	-DSRCDIR=\"$(top_srcdir)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = xfconf-query

xfconf_query_SOURCES = \
	main.c

xfconf_query_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(PLATFORM_CFLAGS)

xfconf_query_LDFLAGS = \
	$(PLATFORM_LDFLAGS)

xfconf_query_LDADD = \
	$(top_builddir)/common/libxfconf-gvaluefuncs.la \
	$(top_builddir)/xfconf/libxfconf-0.la \
	$(GLIB_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(DBUS_GLIB_LIBS)

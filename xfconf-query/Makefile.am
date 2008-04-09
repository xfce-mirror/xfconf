
bin_PROGRAMS = xfconf-query

xfconf_query_SOURCES = \
	main.c

xfconf_query_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	-I$(top_srcdir)/common \
	-DDATADIR=\"$(datadir)\" \
	-DSRCDIR=\"$(top_srcdir)\" \
	-DLOCALEDIR=\"$(localedir)\"

xfconf_query_LDADD = \
	$(top_builddir)/xfconf/libxfconf-0.la \
	$(top_builddir)/common/libxfconf-gvaluefuncs.la \
	$(GLIB_LIBS) \
	$(DBUS_GLIB_LIBS)

INCLUDES = \
	-I$(top_srcdir)

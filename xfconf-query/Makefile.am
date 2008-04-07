
bin_PROGRAMS = xfconf-query

xfconf_query_SOURCES = \
	main.c

xfconf_query_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	-DDATADIR=\"$(datadir)\" \
	-DSRCDIR=\"$(top_srcdir)\" \
	-DLOCALEDIR=\"$(localedir)\"

xfconf_query_LDADD = \
	$(top_builddir)/xfconf/libxfconf-0.la \
	$(GLIB_LIBS) \
	$(DBUS_GLIB_LIBS)

INCLUDES = \
	-I$(top_srcdir)

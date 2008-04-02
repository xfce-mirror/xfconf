bin_PROGRAMS = xfsettingsd

xfsettingsd_SOURCES = \
	main.c \
	registry.c registry.h

xfsettingsd_CFLAGS = \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	-DDATADIR=\"$(datadir)\" \
	-DSRCDIR=\"$(top_srcdir)\" \
	-DLOCALEDIR=\"$(localedir)\"

xfsettingsd_LDADD = \
	$(top_builddir)/xfconf/libxfconf-0.la \
	$(GTK_LIBS) \
	$(GLIB_LIBS) \
	$(DBUS_GLIB_LIBS)

INCLUDES = \
	-I${top_srcdir}

if BUILD_FUSE_MODULE

bin_PROGRAMS = xfconf-fuse

xfconf_fuse_SOURCES = \
	xfconf-fuse.c

xfconf_fuse_CFLAGS = \
	-I$(top_srcdir) \
	$(GLIB_CFLAGS) \
	$(FUSE_CFLAGS)

xfconf_fuse_DEPENDENCIES = \
	$(top_builddir)/xfconf/libxfconf-0.la

xfconf_fuse_LDADD = \
	$(top_builddir)/xfconf/libxfconf-0.la \
	$(GLIB_LIBS) \
	$(FUSE_LIBS)

endif

EXTRA_DIST = \
	xfconf-fuse.c

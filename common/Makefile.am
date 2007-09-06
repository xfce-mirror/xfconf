
noinst_LTLIBRARIES = libxfconf-common.la

libxfconf_common_la_SOURCES = \
	xfconf-marshal.c \
	xfconf-marshal.h \
	xfconf-util.c \
	xfconf-util.h

libxfconf_common_la_CFLAGS = \
	$(GLIB_CFLAGS)

if MAINTAINER_MODE

all-local: xfconf-marshal.c xfconf-marshal.h

xfconf-marshal.h: stamp-xfconf-marshal.h
	@true
stamp-xfconf-marshal.h: xfconf-marshal.list Makefile
	glib-genmarshal --prefix=xfconf_marshal xfconf-marshal.list --header > xfconf-marshal.h
	echo timestamp > $(@F)
xfconf-marshal.c: xfconf-marshal.list Makefile
	echo '#include "xfconf-marshal.h"' > xfconf-marshal.c
	glib-genmarshal --prefix=xfconf_marshal xfconf-marshal.list --body >> xfconf-marshal.c

endif


EXTRA_DIST = \
	xfconf-dbus.xml \
	xfconf-marshal.list

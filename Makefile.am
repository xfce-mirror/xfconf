ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = \
	common \
	xfconf \
	xfconfd \
	xfconf-query \
	po \
	docs \
	tests

if ENABLE_BASH_COMPLETION
SUBDIRS += completions
endif

if XFCONF_ENABLE_GSETTINGS_BACKEND
SUBDIRS += gsettings-backend
endif

.PHONY: ChangeLog

ChangeLog: Makefile
	(GIT_DIR=$(top_srcdir)/.git git log > .changelog.tmp \
	&& mv .changelog.tmp ChangeLog; rm -f .changelog.tmp) \
	|| (touch ChangeLog; echo 'Git directory not found: installing possibly empty changelog.' >&2)

dist-hook: ChangeLog

EXTRA_DIST = \
	m4/introspection.m4 \
	m4/vapigen.m4 \
	meson.build \
	meson_options.txt \
	po/meson.build \
	xfce-revision.h.in \
	$(NULL)

AM_DISTCHECK_CONFIGURE_FLAGS = \
	--enable-gtk-doc \
	--enable-introspection \
	--disable-vala

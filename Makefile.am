SUBDIRS = \
	common \
	xfconf \
	xfconfd \
	xfconf-query \
	xfconf-perl \
	po \
	docs \
	tests

.PHONY: ChangeLog

ChangeLog: Makefile
	(GIT_DIR=$(top_srcdir)/.git git log > .changelog.tmp \
	&& mv .changelog.tmp ChangeLog; rm -f .changelog.tmp) \
	|| (touch ChangeLog; echo 'Git directory not found: installing possibly empty changelog.' >&2)

dist-hook: ChangeLog

EXTRA_DIST = \
	AUTHORS \
	COPYING \
	ChangeLog \
	INSTALL \
	NEWS \
	README

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-gtk-doc

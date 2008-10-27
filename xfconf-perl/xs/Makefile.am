
xfconf_perl_xs_files = \
	Xfconf.xs \
	XfconfBinding.xs \
	XfconfChannel.xs

EXTRA_DIST = \
	$(xfconf_perl_xs_files)

if BUILD_PERL_BINDINGS
all-local:
	if test "$(top_srcdir)" != "$(top_builddir)"; then \
	    for f in $(xfconf_perl_xs_files); do \
	        test -f $$f || $(LN_S) $(srcdir)/$$f $$f; \
	    done; \
	fi
endif

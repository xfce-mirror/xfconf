dnl configure.ac
dnl
dnl xfconf - dbus-based configuration daemon and library for xfce
dnl
dnl Copyright (c) 2007-2011 Brian Tarricone <bjt23@cornell.edu>
dnl Copyright (c) 2011-2020 Xfce Development Team <xfce4-dev@xfce.org>
dnl

dnl version info
m4_define([libxfconf_verinfo], [3:0:0])
m4_define([libxfconf_version_api], [0])
m4_define([copyright_year], [2025])
XDT_VERSION_INIT([4.21.0], [git])

dnl minimum required versions
m4_define([glib_min_version], [2.72.0])
m4_define([libxfce4util_min_version], [4.17.3])

dnl init autoconf
AC_COPYRIGHT([Copyright (c) 2007-copyright_year() The Xfce development team])
AC_INIT([xfconf], [xdt_version], [https://gitlab.xfce.org/xfce/xfconf])
AC_CONFIG_MACRO_DIRS([m4])
AC_PREREQ([2.69])
AC_REVISION([xdt_version_build])
AC_DEFINE([VERSION_FULL], [PACKAGE_VERSION], [Alias for VERSION and PACKAGE_VERSION for meson compatibility])
COPYRIGHT_YEAR=copyright_year()
AC_DEFINE_UNQUOTED([COPYRIGHT_YEAR], ["$COPYRIGHT_YEAR"], [Copyright year])
AC_SUBST([COPYRIGHT_YEAR])

dnl init automake
AM_INIT_AUTOMAKE([1.11 dist-bzip2 tar-ustar no-dist-gzip foreign subdir-objects])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE
AM_SILENT_RULES([yes])

dnl set helper path prefix
AC_ARG_WITH([helper-path-prefix],
            [AS_HELP_STRING([--with-helper-path-prefix=PATH],
                            [Path prefix under which helper executables will be installed (default: $libdir)])],
            [HELPER_PATH_PREFIX="$withval"],
            [HELPER_PATH_PREFIX="$libdir"])
AC_SUBST([HELPER_PATH_PREFIX])


dnl check for basic programs
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LN_S
AC_PROG_SED

LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl check for standard header files
AC_CHECK_HEADERS([errno.h fcntl.h  grp.h locale.h \
                  signal.h stdlib.h string.h \
                  sys/stat.h sys/time.h sys/types.h sys/wait.h \
                  unistd.h])
dnl AC_CHECK_FUNCS([fdwalk getdtablesize setlocale setsid sysconf])
AC_CHECK_FUNCS([fdatasync fsync setlocale])

dnl version information
XFCONF_VERSION=xdt_version
LIBXFCONF_VERSION_API=libxfconf_version_api
LIBXFCONF_VERINFO=libxfconf_verinfo
AC_SUBST(XFCONF_VERSION)
AC_SUBST(LIBXFCONF_VERSION_API)
AC_SUBST(LIBXFCONF_VERINFO)

dnl Check for i18n support
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl Check for gdbus-codegen
XDT_CHECK_PACKAGE_BINARY([GDBUS_CODEGEN], [gio-2.0], [gdbus_codegen], [gdbus-codegen])

dnl ******************************************************************
dnl *** Alternative service name prefix for e.g. Flatpak packaging ***
dnl ******************************************************************
AC_ARG_VAR([XFCONF_SERVICE_NAME_PREFIX],
           [Alternative prefix to org.xfce for the Xfconf service])
test -n "$XFCONF_SERVICE_NAME_PREFIX" || XFCONF_SERVICE_NAME_PREFIX='org.xfce'

AC_SUBST([XFCONF_SERVICE_PATH_PREFIX],
         [$(printf '%s' $XFCONF_SERVICE_NAME_PREFIX | $SED -E 's%(^|\.)%/%g')])
AC_DEFINE_UNQUOTED(XFCONF_SERVICE_NAME_PREFIX, "$XFCONF_SERVICE_NAME_PREFIX",
                   Name prefix for the Xfconf service)
AC_DEFINE_UNQUOTED(XFCONF_SERVICE_PATH_PREFIX, "$XFCONF_SERVICE_PATH_PREFIX",
                   Path prefix for the Xfconf service)

dnl required
XDT_CHECK_PACKAGE([GLIB], [gobject-2.0], [glib_min_version])
XDT_CHECK_PACKAGE([GIO], [gio-2.0], [glib_min_version])
XDT_CHECK_PACKAGE([GIO_UNIX], [gio-unix-2.0], [glib_min_version])
XDT_CHECK_PACKAGE([GTHREAD], [gthread-2.0], [glib_min_version])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [libxfce4util_min_version])

dnl check for xdt-gen-visibility
AC_PATH_PROG([XDT_GEN_VISIBILITY], [xdt-gen-visibility])

dnl check alignment for struct returns
AC_DEFUN([BT_GLIB_CHECK_ALIGNOF],
[
    saved_CFLAGS="$CFLAGS"
    saved_LIBS="$LIBS"
    CFLAGS="$CFLAGS $GLIB_CFLAGS"
    LIBS="$LIBS $GLIB_LIBS"
    AC_CHECK_ALIGNOF([$1], [#include <glib.h>])
    CFLAGS="$saved_CFLAGS"
    LIBS="$saved_LIBS"
])
BT_GLIB_CHECK_ALIGNOF(guchar)
BT_GLIB_CHECK_ALIGNOF(gchar)
BT_GLIB_CHECK_ALIGNOF(guint16)
BT_GLIB_CHECK_ALIGNOF(gint16)
BT_GLIB_CHECK_ALIGNOF(guint32)
BT_GLIB_CHECK_ALIGNOF(gint32)
BT_GLIB_CHECK_ALIGNOF(guint64)
BT_GLIB_CHECK_ALIGNOF(gint64)
BT_GLIB_CHECK_ALIGNOF(gfloat)
BT_GLIB_CHECK_ALIGNOF(gdouble)
BT_GLIB_CHECK_ALIGNOF(gboolean)
BT_GLIB_CHECK_ALIGNOF(gpointer)

dnl gtk-doc
GTK_DOC_CHECK([1.20])

dnl backends

dnl always build the perchannel-xml backend
xfconf_backend_perchannel_xml=yes
AM_CONDITIONAL([BUILD_XFCONF_BACKEND_PERCHANNEL_XML],
               [test "x$xfconf_backend_perchannel_xml" = "xyes"])
if test "x$xfconf_backend_perchannel_xml" = "xyes"; then
    AC_DEFINE([BUILD_XFCONF_BACKEND_PERCHANNEL_XML], [1],
              [Define if the perchannel-xml backend should be built])
fi

dnl check for GSettings backend support
AC_ARG_ENABLE([gsettings_backend], AS_HELP_STRING([--disable-gsettings-backend], [Don't build GSettings backend module]),
  [ac_gsettings_backend=$enableval], [ac_gsettings_backend=yes])
AC_MSG_CHECKING([whether to build the GSettings backend module])
AM_CONDITIONAL([XFCONF_ENABLE_GSETTINGS_BACKEND], [test x"$ac_gsettings_backend" = x"yes"])
AC_MSG_RESULT([$ac_gsettings_backend])

dnl GModule is required in order to build the GSettings backend module
if test "x$ac_gsettings_backend"; then
  XDT_CHECK_PACKAGE([GMODULE], [gmodule-2.0], [glib_min_version])
  GIO_MODULE_DIR=$libdir/gio/modules
  AC_SUBST(GIO_MODULE_DIR)
fi

dnl check for GObject Introspection
GOBJECT_INTROSPECTION_CHECK([1.72.0])

dnl check for Vala support
VAPIGEN_CHECK()

dnl check for debugging support
XDT_FEATURE_DEBUG([xdt_debug_default])

dnl gtk-doc is broken
XDT_SUPPORTED_FLAGS([GTK_DOC_EXTRA_CFLAGS], [-Wno-sign-compare])
AC_SUBST([GTK_DOC_EXTRA_CFLAGS])

AC_ARG_ENABLE([checks],
              AS_HELP_STRING([--enable-checks],
                             [Enables some runtime checks for libxfconf that print debugging information at a slight cost to library code size and speed]),
              [enable_checks=$enableval])
if test "x$enable_checks" = "xyes"; then
    AC_DEFINE([XFCONF_ENABLE_CHECKS], [1],
              [Define if runtime checks should be performed])
fi

dnl check whether the compiler supports the visibility attribute
AC_ARG_ENABLE([visibility],
              AS_HELP_STRING([--disable-visibility],
                             [Do not use ELF visibility attributes]),
              [enable_visibility=$enableval], [enable_visibility=yes])
have_gnuc_visibility=no
if test "x$enable_visibility" != "xno"; then
  XDT_SUPPORTED_FLAGS([xdt_vis_test_cflags], [-Wall -Werror -Wno-unused-parameter -fvisibility=hidden])
  saved_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $xdt_vis_test_cflags"
  AC_MSG_CHECKING([whether $CC supports the GNUC visibility attribute])
  AC_COMPILE_IFELSE([AC_LANG_SOURCE(
  [
    void test_default(void);
    void test_hidden(void);
    void __attribute__((visibility("default"))) test_default(void) {}
    void __attribute__((visibility("hidden"))) test_hidden(void) {}
    int main(int argc, char **argv) {
      test_default();
      test_hidden();
      return 0;
    }
  ])],
  [
    have_gnuc_visibility=yes
    AC_MSG_RESULT([yes])
  ], 
  [
    AC_MSG_RESULT([no])
  ]) 
  CFLAGS="$saved_CFLAGS"
fi
if test "x$have_gnuc_visibility" = "xyes"; then
  CPPFLAGS="$CPPFLAGS -DENABLE_SYMBOL_VISIBILITY=1"
  CFLAGS="$CFLAGS -fvisibility=hidden"
  AC_DEFINE([XFCONF_EXPORT], [__attribute__((visibility("default")))], [defines how to decorate public symbols while building])
else
  AC_DEFINE([XFCONF_EXPORT], [], [defines how to decorate public symbols while building])
fi

dnl add -O1 and --as-needed to LDFLAGS if supported
XDT_FEATURE_LINKER_OPTS

AC_ARG_ENABLE([profiling],
              AS_HELP_STRING([--enable-profiling],
                             [Enable gprof profiling support]),
              [enable_profiling=$enableval])
if test "x$enable_profiling" = "xyes"; then
    AC_DEFINE([XFCONF_ENABLE_PROFILING], [1],
              [Define if gprof profiling should be compiled in])
fi
AM_CONDITIONAL([ENABLE_PROFILING], [test "x$enable_profiling" = "xyes"])

dnl Bash completion for xfconf-query.
AC_ARG_WITH([bash-completion-dir],
            AS_HELP_STRING([--with-bash-completion-dir[=PATH]],
                           [Install the bash-completion script in this directory. @<:@default=yes@:>@]),
            [],
            [with_bash_completion_dir=yes])
AS_IF([test "x$with_bash_completion_dir" = xyes],
          [AC_MSG_CHECKING([for bash-completion directory])
           AS_IF([test "x$PKG_CONFIG" != x \
              && bash_completion_prefix=`$PKG_CONFIG --print-errors --variable=prefix bash-completion 2>&AS_MESSAGE_LOG_FD` \
              && bash_completion_dir=`$PKG_CONFIG --print-errors --variable=completionsdir bash-completion 2>&AS_MESSAGE_LOG_FD`],
             [bash_completion_dir="${bash_completion_dir#"$bash_completion_prefix"}"
              bash_completion_dir="${bash_completion_dir#/}"
              BASH_COMPLETION_DIR='${prefix}'/"$bash_completion_dir"],
             [BASH_COMPLETION_DIR='${datadir}/bash-completion/completions'])
           AC_MSG_RESULT([$BASH_COMPLETION_DIR])],
          [BASH_COMPLETION_DIR="$with_bash_completion_dir"])
AC_SUBST([BASH_COMPLETION_DIR])
AM_CONDITIONAL([ENABLE_BASH_COMPLETION],
               [test "x$with_bash_completion_dir" != xno])

AC_CONFIG_FILES([
Makefile
common/Makefile
docs/Makefile
docs/reference/Makefile
docs/spec/Makefile
po/Makefile.in
tests/Makefile
tests/set-properties/Makefile
tests/has-properties/Makefile
tests/get-properties/Makefile
tests/reset-properties/Makefile
tests/object-bindings/Makefile
tests/property-changed-signal/Makefile
tests/others/Makefile
tests/list-channels/Makefile
tests/tests-end/Makefile
xfconf/Makefile
xfconf/libxfconf-0.pc
xfconf-query/Makefile
xfconfd/Makefile
gsettings-backend/Makefile
completions/Makefile
])
AC_OUTPUT

dnl ***************************
dnl *** Print configuration ***
dnl ***************************
echo
echo "Build Configuration:"
echo
echo "  * GSettings Backend:         $ac_gsettings_backend"
echo "  * GObject Introspection:     $enable_introspection"
echo "  * Vala support:              $enable_vala"
if test "x$with_bash_completion_dir" != "xno"; then
echo "  * Bash completion path:      $BASH_COMPLETION_DIR"
else
echo "  * Bash completion:           no"
fi
echo
echo "  * Debug:                     $enable_debug"
echo

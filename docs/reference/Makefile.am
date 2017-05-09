## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.8

# The name of the module.
DOC_MODULE=xfconf

# The top-level SGML file.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.xml

# Extra options to supply to gtkdoc-scan
SCAN_OPTIONS=--deprecated-guards="XFCONF_DISABLE_DEPRECATED"

# The directory containing the source code. Relative to $(srcdir)
DOC_SOURCE_DIR=../..

# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-fixref
FIXXREF_OPTIONS=--module=$(DOC_MODULE)

# Used for dependencies
HFILE_GLOB = \
	$(top_srcdir)/xfconf/*.h \
	$(top_srcdir)/xfconfd/*.h
CFILE_GLOB = \
	$(top_srcdir)/xfconf/*.c \
	$(top_srcdir)/xfconfd/*.c \
	$(top_srcdir)/common/xfconf-errors.c

# Header files to ignore when scanning
IGNORE_HFILES= \
	config.h \
	xfconf-backend-factory.h \
	xfconf-backend-perchannel-xml.h \
	xfconf-daemon.h \
	xfconf-gdbus-bindings.h \
	xfconf-gdbus-bindings.c \
	xfconf-marshal.h \
	xfconf-private.h \
	xfconf-common-private.h

# Extra files to add when scanning (relative to $srcdir)
EXTRA_HFILES=

# Images to copy into HTML directory
HTML_IMAGES = 

# Extra SGML files that are included by DOC_MAIN_SGML_FILE
content_files = \
	version.xml

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(GLIB_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(GIO_UNIX_CFLAGS) \
	$(GTK_DOC_EXTRA_CFLAGS)

GTKDOC_LIBS = \
	$(top_builddir)/xfconf/libxfconf-$(LIBXFCONF_VERSION_API).la \
	$(top_builddir)/xfconfd/xfconfd-xfconf-backend.o \
	$(GLIB_LIBS) \
	$(GOBJECT_LIBS)

include $(top_srcdir)/gtk-doc.make

# Other files to distribute
EXTRA_DIST += \
	version.xml.in

# manual pages
man_MANS =

if MAINTAINER_MODE
%.1: %.xml
	xsltproc -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<
endif

# required for gtk-doc
dist-hook: all

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
